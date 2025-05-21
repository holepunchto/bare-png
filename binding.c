#include <assert.h>
#include <bare.h>
#include <js.h>
#include <png.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <utf.h>

typedef struct {
  uint8_t *data;
  size_t len;
  size_t offset;
} bare_png_reader_t;

typedef struct {
  uint8_t *data;
  size_t len;
  size_t capacity;
} bare_png_writer_t;

typedef struct {
  jmp_buf jump;
  char message[256];
} bare_png_error_t;

static void
bare_png__on_error(png_structp png, png_const_charp message) {
  bare_png_error_t *error = (bare_png_error_t *) png_get_error_ptr(png);

  snprintf(error->message, sizeof(error->message), "%s", message);

  longjmp(error->jump, 1);
}

static void
bare_png__on_finalize(js_env_t *env, void *data, void *finalize_hint) {
  free(data);
}

static void
bare_png__on_read(png_structp png, png_bytep data, png_size_t len) {
  bare_png_reader_t *reader = (bare_png_reader_t *) png_get_io_ptr(png);

  if (reader->offset + len <= reader->len) {
    memcpy(data, reader->data + reader->offset, len);

    reader->offset += len;
  } else {
    png_error(png, "Out of bounds");
  }
}

static void
bare_png__on_write(png_structp png, png_bytep data, png_size_t len) {
  bare_png_writer_t *writer = (bare_png_writer_t *) png_get_io_ptr(png);

  if (writer->len + len > writer->capacity) {
    writer->capacity = (writer->len + len) * 2;

    writer->data = realloc(writer->data, writer->capacity);
  }

  memcpy(writer->data + writer->len, data, len);

  writer->len += len;
}

static void
bare_png__on_flush(png_structp png) {}

static js_value_t *
bare_png_decode(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 1);

  uint8_t *png;
  size_t len;
  err = js_get_typedarray_info(env, argv[0], NULL, (void **) &png, &len, NULL, NULL);
  assert(err == 0);

  bare_png_error_t error;

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, &error, bare_png__on_error, NULL);

  png_infop info_ptr = png_create_info_struct(png_ptr);

  if (setjmp(error.jump)) {
    err = js_throw_error(env, NULL, error.message);
    assert(err == 0);

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    return NULL;
  }

  bare_png_reader_t reader = {png, len, 0};

  png_set_read_fn(png_ptr, &reader, bare_png__on_read);

  png_read_info(png_ptr, info_ptr);

  int width = png_get_image_width(png_ptr, info_ptr);
  int height = png_get_image_height(png_ptr, info_ptr);
  int color_type = png_get_color_type(png_ptr, info_ptr);
  int bit_depth = png_get_bit_depth(png_ptr, info_ptr);

  if (bit_depth == 16) {
    png_set_strip_16(png_ptr);
  }

  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png_ptr);

  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
    png_set_expand_gray_1_2_4_to_8(png_ptr);
  }

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
    png_set_tRNS_to_alpha(png_ptr);
  }

  if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY) {
    png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);
  }

  if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    png_set_gray_to_rgb(png_ptr);
  }

  png_read_update_info(png_ptr, info_ptr);

  png_bytep *rows = malloc(sizeof(png_bytep) * height);

  uint8_t *data = malloc(width * height * 4);

  for (int y = 0; y < height; y++) {
    rows[y] = data + y * width * 4;
  }

  png_read_image(png_ptr, rows);

  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

  free(rows);

  js_value_t *result;
  err = js_create_object(env, &result);
  assert(err == 0);

#define V(n) \
  { \
    js_value_t *val; \
    err = js_create_int64(env, n, &val); \
    assert(err == 0); \
    err = js_set_named_property(env, result, #n, val); \
    assert(err == 0); \
  }

  V(width);
  V(height);
#undef V

  js_value_t *buffer;
  err = js_create_external_arraybuffer(env, data, len, bare_png__on_finalize, NULL, &buffer);
  assert(err == 0);

  err = js_set_named_property(env, result, "data", buffer);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_png_encode(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 3;
  js_value_t *argv[3];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 3);

  uint8_t *data;
  err = js_get_typedarray_info(env, argv[0], NULL, (void **) &data, NULL, NULL, NULL);
  assert(err == 0);

  int64_t width;
  err = js_get_value_int64(env, argv[1], &width);
  assert(err == 0);

  int64_t height;
  err = js_get_value_int64(env, argv[2], &height);
  assert(err == 0);

  bare_png_error_t error;

  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, &error, bare_png__on_error, NULL);

  png_infop info_ptr = png_create_info_struct(png_ptr);

  if (setjmp(error.jump)) {
    err = js_throw_error(env, NULL, error.message);
    assert(err == 0);

    png_destroy_write_struct(&png_ptr, &info_ptr);

    return NULL;
  }

  bare_png_writer_t writer = {NULL, 0, 0};

  png_set_write_fn(png_ptr, &writer, bare_png__on_write, bare_png__on_flush);

  png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  png_write_info(png_ptr, info_ptr);

  png_bytep *rows = malloc(sizeof(png_bytep) * height);

  for (int y = 0; y < height; y++) {
    rows[y] = data + y * width * 4;
  }

  png_write_image(png_ptr, rows);
  png_write_end(png_ptr, NULL);

  png_destroy_write_struct(&png_ptr, &info_ptr);

  free(rows);

  js_value_t *result;
  err = js_create_external_arraybuffer(env, writer.data, writer.len, bare_png__on_finalize, NULL, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_png_exports(js_env_t *env, js_value_t *exports) {
  int err;

#define V(name, fn) \
  { \
    js_value_t *val; \
    err = js_create_function(env, name, -1, fn, NULL, &val); \
    assert(err == 0); \
    err = js_set_named_property(env, exports, name, val); \
    assert(err == 0); \
  }

  V("decode", bare_png_decode)
  V("encode", bare_png_encode)
#undef V

  return exports;
}

BARE_MODULE(bare_png, bare_png_exports)
