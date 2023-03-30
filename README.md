# nbsdecoder.js

[![Node.js CI](https://github.com/Fastcode/nbsdecoder.js/actions/workflows/node.js.yml/badge.svg?branch=main)](https://github.com/Fastcode/nbsdecoder.js/actions/workflows/node.js.yml)

Node.js module for interacting with [NUClear Binary Stream](https://nubook.nubots.net/system/foundations/nuclear#nbs-files) files.

## Installation

The package contains a native module, so you'll need a working C++ compiler on your system to install and build it.

```
npm install nbsdecoder.js --save
```

## Usage

This package contains classes for reading and writing nbs files.

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

The following example shows a typical usage pattern of creating an encoder with an nbs file path and writing a packet to it.

```js
const { NbsEncoder } = require('nbsdecoder.js');

// Create an encoder instance
const encoder = new NbsEncoder('/path/to/new/file.nbs');

// Create a packet to write to the file
const packet = {
  // Timestamp that the packet was emitted
  timestamp: { seconds: 2000, nanos: 0 },

  // The nuclear hash of the message type name (In this example 'message.Ping')
  type: Buffer.from('8ce1582fa0eadc84', 'hex'),

  // Optional subtype of the packet
  subtype: 0,

  // Bytes of the payload
  payload: Buffer.from('Message', 'utf8'),
};

// Write the packet to the file
encoder.write(packet);

// Close the file reader
encoder.close();
```

## API

See [`nbsdecoder.d.ts`](./nbsdecoder.d.ts) for API and types.
