# nbsdecoder.js

[![Node.js CI](https://github.com/Fastcode/nbsdecoder.js/actions/workflows/node.js.yml/badge.svg?branch=main)](https://github.com/Fastcode/nbsdecoder.js/actions/workflows/node.js.yml)

Node.js module for interacting with [NUClear Binary Stream](https://nubook.nubots.net/system/foundations/nuclear#nbs-files) files.

## Installation

The package contains a native module, so you'll need a working C++ compiler on your system to install and build it.

```
npm install nbsdecoder.js --save
```

## Usage

The following example shows a typical usage pattern of creating a decoder with nbs file paths, and reading types, timestamps, and data packets.

```js
const { NbsDecoder } = require('nbsdecoder.js');

// Create a decoder instance
const decoder = new NbsDecoder([
  '/path/to/file/a.nbs',
  '/path/to/file/b.nbs',
  '/path/to/file/c.nbs',
]);

// Get a list of all types present in the NBS files, and get the first type
const types = decoder.getAvailableTypes();
const firstType = types[0];

// Set the timestamp range for the first available type
const [start, end] = decoder.getTimestampRange(firstType);

// Get the packet at the given timestamp for the first type
const packets = decoder.getPackets(start, [firstType]);

// Log all the values for inspection
console.log({ types, firstType, start, end, packets });
```

## API

See [`nbsdecoder.d.ts`](./nbsdecoder.d.ts) for API and types.
