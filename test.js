const test = require('brittle')
const png = require('.')

test('decode .png', (t) => {
  const image = require('./test/fixtures/grapefruit.png', {
    with: { type: 'binary' }
  })

  t.comment(png.decode(image))
})

test('encode .png', (t) => {
  const image = require('./test/fixtures/grapefruit.png', {
    with: { type: 'binary' }
  })

  const decoded = png.decode(image)

  t.comment(png.encode(decoded))
})

test('decode rejects oversized dimensions without crashing', (t) => {
  // Regression: width * height * 4 used to overflow `int` and produce
  // a wrapped, undersized malloc — heap corruption on per-row writes.
  const malicious = buildPng(65536, 65536)

  t.exception(() => png.decode(malicious))
})

function buildPng(width, height) {
  const sig = Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a])

  const ihdr = Buffer.alloc(13)
  ihdr.writeUInt32BE(width, 0)
  ihdr.writeUInt32BE(height, 4)
  ihdr[8] = 8 // bit depth
  ihdr[9] = 6 // color type: RGBA

  const idat = Buffer.from([0x78, 0x9c, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01])

  return Buffer.concat([
    sig,
    chunk('IHDR', ihdr),
    chunk('IDAT', idat),
    chunk('IEND', Buffer.alloc(0))
  ])
}

function chunk(type, data) {
  const len = Buffer.alloc(4)
  len.writeUInt32BE(data.length, 0)

  const typeBuf = Buffer.from(type, 'ascii')

  const crcBuf = Buffer.alloc(4)
  crcBuf.writeUInt32BE(crc32(Buffer.concat([typeBuf, data])), 0)

  return Buffer.concat([len, typeBuf, data, crcBuf])
}

function crc32(buf) {
  let crc = 0xffffffff

  for (let i = 0; i < buf.length; i++) {
    crc ^= buf[i]

    for (let j = 0; j < 8; j++) {
      crc = (crc >>> 1) ^ (0xedb88320 & -(crc & 1))
    }
  }

  return (crc ^ 0xffffffff) >>> 0
}
