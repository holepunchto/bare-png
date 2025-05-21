# bare-png

PNG support for Bare.

```
npm i bare-png
```

## Usage

```js
const png = require('bare-png')

const image = require('./my-image.png', { with: { type: 'binary' } })

const decoded = png.decode(image)
// {
//   width: 200,
//   height: 400,
//   data: <Buffer>
// }

const encoded = png.encode(decoded)
// <Buffer>
```

## License

Apache-2.0
