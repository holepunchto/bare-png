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
