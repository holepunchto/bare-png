const binding = require('./binding')

exports.decode = function decode(image) {
  const { width, height, data } = binding.decode(image)

  return {
    width,
    height,
    data: Buffer.from(data)
  }
}

exports.encode = function encode(image) {
  const buffer = binding.encode(image.data, image.width, image.height)

  return Buffer.from(buffer)
}
