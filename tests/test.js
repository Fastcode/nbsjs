const { on } = require('events');
const fs = require('fs');
const path = require('path');
const { test } = require('uvu');
const assert = require('uvu/assert');
const { NbsDecoder } = require('..');

/** Convert the given timestamp object to a BigInt of nanoseconds */
function tsToBigInt(ts) {
  return BigInt(ts.seconds) * BigInt(1e9) + BigInt(ts.nanos);
}

/** A Jest-like snapshot assertion */
function assertSnapshot(value, fileName, message) {
  const filePath = path.join(__dirname, 'snapshots', fileName);
  const json = JSON.stringify(value, null, 2);

  if (fs.existsSync(filePath)) {
    const snapshot = fs.readFileSync(filePath, 'utf8');
    assert.snapshot(json, snapshot, message);
  } else {
    fs.writeFileSync(filePath, json, 'utf8');
    assert.ok(false, 'Snapshot file not found. Created one.');
  }
}

const samplesDir = path.join(__dirname, 'sample');

// What's in the sample nbs files:
// For each sample file:
//   - There are 300 messages, with 3 message types: `message.Ping`, `message.Pong`, and `message.Pang`
//   - The messages are repeated in sets of three: ping, pong, pang
//   - Each message in a set has the payload "$type.$i" where $type is "ping", "pong" or "pang",
//     and $i is an index in the range specified in the file name. The index is the same for each message in the set.
//     E.g. for the file "sample-001-300.nbs" $i goes from 1 to 100.
//   - Pang messages also have a subtype: 100 or 200. 100 is used for even-indexed pang messages, 200 for odd-indexed.
// The timestamps begin at 1000 seconds after epoch for the first message in the first file and increment by 1 second between messages.
const decoder = new NbsDecoder([
  path.join(samplesDir, 'sample-000-300.nbs'),
  path.join(samplesDir, 'sample-300-600.nbs'),
  path.join(samplesDir, 'sample-600-900.nbs'),
]);

const pingType = Buffer.from('8ce1582fa0eadc84', 'hex'); // nuclear hash of 'message.Ping'
const pongType = Buffer.from('37c56336526573bb', 'hex'); // nuclear hash of 'message.Pong'
const pangType = Buffer.from('c63bd829ef39b750', 'hex'); // nuclear hash of 'message.Pang'

test('NbsDecoder constructor throws for invalid arguments', () => {
  assert.throws(
    () => {
      new NbsDecoder();
    },
    /missing argument `paths`: provide an array of nbs file paths/,
    'NbsDecoder() constructor throws without path argument'
  );

  assert.throws(
    () => {
      new NbsDecoder(false);
    },
    /invalid argument `paths`: expected array/,
    'NbsDecoder() constructor throws for non-array argument'
  );

  assert.throws(
    () => {
      new NbsDecoder([]);
    },
    /invalid argument `paths`: expected non-empty array/,
    'NbsDecoder() constructor throws for empty array argument'
  );

  assert.throws(
    () => {
      new NbsDecoder(['/invalid/file/path.nbs']);
    },
    /nbs index not found for file/,
    'NbsDecoder() constructor throws for non-existent paths'
  );
});

test('NbsDecoder.getAvailableTypes() returns a list of all the types available in the nbs files', () => {
  const types = decoder.getAvailableTypes();

  assert.equal(types.length, 4, 'there are 4 different (type,subtype) in the sample files');

  assertSnapshot(
    types,
    'get-available-types.json',
    'available types snapshot matches expected types in sample files'
  );
});

test('NbsDecoder.getTimestampRange() throws for invalid arguments', () => {
  assert.throws(
    () => {
      decoder.getTimestampRange(false);
    },
    /invalid type for argument `typeSubtype`: expected object/,
    'NbsDecoder.getTimestampRange() throws for invalid argument type'
  );

  assert.throws(
    () => {
      decoder.getTimestampRange({});
    },
    /invalid type for argument `typeSubtype`: expected object with `type` and `subtype` keys/,
    'NbsDecoder.getTimestampRange() throws for object without `type` or `subtype` keys'
  );

  assert.throws(
    () => {
      decoder.getTimestampRange({ type: 'some-type' });
    },
    /invalid type for argument `typeSubtype`: expected object with `type` and `subtype` keys/,
    'NbsDecoder.getTimestampRange() throws for object without `subtype` key'
  );

  assert.throws(
    () => {
      decoder.getTimestampRange({ subtype: 0 });
    },
    /invalid type for argument `typeSubtype`: expected object with `type` and `subtype` keys/,
    'NbsDecoder.getTimestampRange() throws for object without `type` key'
  );

  assert.throws(
    () => {
      decoder.getTimestampRange({ type: false, subtype: 0 });
    },
    /invalid type for argument `typeSubtype`: invalid `.type`: expected a string or Buffer/,
    'NbsDecoder.getTimestampRange() throws for object with invalid `type`'
  );

  assert.throws(
    () => {
      decoder.getTimestampRange({ type: 'some-type', subtype: false });
    },
    /invalid type for argument `typeSubtype`: invalid `.subtype`: expected number/,
    'NbsDecoder.getTimestampRange() throws for object with invalid `subtype`'
  );
});

test('NbsDecoder.getTimestampRange() without arguments returns the timestamp range across all types', () => {
  const [start, end] = decoder.getTimestampRange();

  assert.equal(start, { seconds: 1000, nanos: 0 }, 'start timestamp is 1000 seconds after epoch');

  assert.equal(
    end,
    { seconds: 1000 + 899, nanos: 0 }, // There are 900 messages in the sample files, spaced 1 seconds apart, with a total range of 899 seconds
    'end timestamp is 1000 seconds after epoch plus 300 seconds'
  );
});

test('NbsDecoder.getTimestampRange() with type argument returns the timestamp range of the given type', () => {
  const [start, end] = decoder.getTimestampRange({ type: pingType, subtype: 0 });

  assert.equal(
    start,
    { seconds: 1000, nanos: 0 }, // Ping is the first message in the file, starting at 1000 seconds
    'start timestamp for ping messages is 1000 seconds after epoch'
  );

  assert.equal(
    end,
    { seconds: 1897, nanos: 0 }, // The last set of messages in the files are (1897, 1898, 1899), and ping is the first of those
    'end timestamp for ping messages is 1897 seconds after epoch'
  );
});

test('NbsDecoder.getPackets() throws for invalid arguments', () => {
  // Invalid `timestamp` argument
  assert.throws(
    () => {
      decoder.getPackets();
    },
    /invalid type for argument `timestamp`: expected positive number or BigInt/,
    'NbsDecoder.getPackets() throws for missing `timestamp` argument'
  );

  assert.throws(
    () => {
      decoder.getPackets(false);
    },
    /invalid type for argument `timestamp`: expected positive number or BigInt/,
    'NbsDecoder.getPackets() throws for missing incorrect `timestamp` argument type'
  );

  // Invalid `types` argument
  assert.throws(
    () => {
      decoder.getPackets(0, false);
    },
    /invalid type for argument `types`: expected array or undefined/,
    'NbsDecoder.getPackets() throws for missing incorrect `types` argument type'
  );

  assert.throws(
    () => {
      decoder.getPackets(0, false);
    },
    /invalid type for argument `types`: expected array or undefined/,
    'NbsDecoder.getPackets() throws for missing incorrect `types` argument type'
  );

  // Invalid item in `types` array
  assert.throws(
    () => {
      decoder.getPackets(0, [false]);
    },
    /invalid item type in `types` array: expected object/,
    'NbsDecoder.getPackets() throws for invalid argument type'
  );

  assert.throws(
    () => {
      decoder.getPackets(0, [{}]);
    },
    /invalid item type in `types` array: expected object with `type` and `subtype` keys/,
    'NbsDecoder.getPackets() throws for object without `type` or `subtype` keys'
  );

  // Invalid type, subtype object in `types` array
  assert.throws(
    () => {
      decoder.getPackets(0, [{ type: 'some-type' }]);
    },
    /invalid item type in `types` array: expected object with `type` and `subtype` keys/,
    'NbsDecoder.getPackets() throws for object without `subtype` key'
  );

  assert.throws(
    () => {
      decoder.getPackets(0, [{ subtype: 0 }]);
    },
    /invalid item type in `types` array: expected object with `type` and `subtype` keys/,
    'NbsDecoder.getPackets() throws for object without `type` key'
  );

  assert.throws(
    () => {
      decoder.getPackets(0, [{ type: false, subtype: 0 }]);
    },
    /invalid item type in `types` array: invalid `.type`: expected a string or Buffer/,
    'NbsDecoder.getPackets() throws for object with invalid `type`'
  );

  assert.throws(
    () => {
      decoder.getPackets(0, [{ type: 'some-type', subtype: false }]);
    },
    /invalid item type in `types` array: invalid `.subtype`: expected number/,
    'NbsDecoder.getPackets() throws for object with invalid `subtype`'
  );
});

test('NbsDecoder.getPackets() accepts `number`, `BigInt` or `{ seconds, nanos }` for timestamp argument', () => {
  const packetsFromNumber = decoder.getPackets(1500 * 1e9);
  const packetsFromBigInt = decoder.getPackets(1500n * BigInt(1e9));
  const packetsFromTimestampObject = decoder.getPackets({ seconds: 1500, nanos: 0 });

  assert.equal(packetsFromNumber, packetsFromBigInt);
  assert.equal(packetsFromNumber, packetsFromTimestampObject);
});

test('NbsDecoder.getPackets() returns packets of all types at the given timestamp if no types are given', () => {
  const knownStartTime = 1000n * BigInt(1e9);

  const packets = decoder.getPackets(knownStartTime);

  assertSnapshot(packets, 'decoder-get-packets-all-types.json');
});

test('NbsDecoder.getPackets() returns returns an empty list given an empty list of types', () => {
  const knownStartTime = 1000n;

  const packets = decoder.getPackets(knownStartTime, []);

  assert.equal(packets.length, 0, '0 packets was returned for 0 requested types');
});

test('NbsDecoder.getPackets() returns packets of the given types at the given timestamp', () => {
  const knownStartTime = {
    seconds: 1000,
    nanos: 0,
  };

  const packets = decoder.getPackets(knownStartTime, [
    { type: pingType, subtype: 0 },
    { type: pongType, subtype: 0 },
  ]);

  assert.equal(
    packets[0],
    {
      timestamp: knownStartTime,
      type: pingType,
      subtype: 0,
      payload: Buffer.from('ping.0', 'utf8'),
    },
    'the first packet at the start is the Ping packet with payload "ping.0"'
  );

  assert.equal(
    packets[1],
    {
      timestamp: knownStartTime,
      type: pongType,
      subtype: 0,
      payload: undefined,
    },
    'the second packet at the start, given the requested types, is an empty Pong packet'
  );

  assert.equal(packets.length, 2, 'two types were requested, so two packets are returned');
});

test('NbsDecoder.getPackets() allows for specifying types using a string of the message name', () => {
  const knownStartTime = {
    seconds: 1000,
    nanos: 0,
  };

  const packetsFromTypeBuffer = decoder.getPackets(knownStartTime, [
    { type: pingType, subtype: 0 },
    { type: pongType, subtype: 0 },
  ]);

  const packetsFromTypeString = decoder.getPackets(knownStartTime, [
    { type: 'message.Ping', subtype: 0 },
    { type: 'message.Pong', subtype: 0 },
  ]);

  assert.equal(packetsFromTypeBuffer, packetsFromTypeString);
});

test('NbsDecoder.getPackets() returned packets are at or before the given timestamp', () => {
  const [start, end] = decoder.getTimestampRange();

  const midTimestamp = (tsToBigInt(start) + tsToBigInt(end)) / 2n;

  const packets = decoder.getPackets(midTimestamp, [{ type: pingType, subtype: 0 }]);

  assert.ok(
    tsToBigInt(packets[0].timestamp) <= midTimestamp,
    'the return packet for a timestamp is before or at the requested timestamp'
  );

  assert.equal(packets[0], {
    timestamp: {
      seconds: 1447, // Known: closest ping message timestamp to the requested mid timestamp (which is 1449s)
      nanos: 0,
    },
    type: pingType,
    subtype: 0,
    payload: Buffer.from('ping.349', 'utf8'),
  });

  assert.equal(packets.length, 1, '1 packet was returned for 1 requested type');
});

test('NbsDecoder.getPackets() returns empty packets when given a timestamp earlier than the start', () => {
  const [start] = decoder.getTimestampRange();

  const beforeStart = {
    seconds: start.seconds - 1,
    nanos: 1e9 - 1,
  };

  const packets = decoder.getPackets(beforeStart);

  assert.equal(packets[0], {
    timestamp: beforeStart,
    type: pangType,
    subtype: 100,
    payload: undefined,
  });

  assert.equal(packets[1], {
    timestamp: beforeStart,
    type: pangType,
    subtype: 200,
    payload: undefined,
  });

  assert.equal(packets[2], {
    timestamp: beforeStart,
    type: pingType,
    subtype: 0,
    payload: undefined,
  });

  assert.equal(packets[3], {
    timestamp: beforeStart,
    type: pongType,
    subtype: 0,
    payload: undefined,
  });
});

test('NbsDecoder.getPackets() returns the last packet of each type when given a timestamp past the end', () => {
  const [, end] = decoder.getTimestampRange();

  const afterEnd = {
    seconds: end.seconds + 1,
    nanos: end.nanos,
  };

  const packets = decoder.getPackets(afterEnd);

  assert.equal(packets[0], {
    timestamp: { seconds: 1896, nanos: 0 },
    type: pangType,
    subtype: 100,
    payload: Buffer.from('pang.698', 'utf8'),
  });

  assert.equal(packets[1], {
    timestamp: { seconds: 1899, nanos: 0 },
    type: pangType,
    subtype: 200,
    payload: Buffer.from('pang.699', 'utf8'),
  });

  assert.equal(packets[2], {
    timestamp: { seconds: 1897, nanos: 0 },
    type: pingType,
    subtype: 0,
    payload: Buffer.from('ping.699', 'utf8'),
  });

  assert.equal(packets[3], {
    timestamp: { seconds: 1898, nanos: 0 },
    type: pongType,
    subtype: 0,
    payload: Buffer.from('pong.699', 'utf8'),
  });
});

const testArray = [{type: pingType, subtype: 0}, {type: pongType, subtype: 0}, {type: pangType, subtype: 100}, {type: pangType, subtype: 200}];

/* Check step = 0 and the returned frame is the same as input. */
test('NbsDecoder.nextTimestamp() outputs the exact same timestamp as input given steps = 0', () => {
  const [start, end] = decoder.getTimestampRange({ type: pingType, subtype: 0 });
  const new_timestamp = decoder.nextTimestamp(start, { type: pingType, subtype: 0} , 0);

    assert.equal(
      tsToBigInt(new_timestamp), tsToBigInt(start),
      'With steps = 0, input start timestamp should be equal to the returned timestamp for single type'
    )

});

/* Check stepping forward by one timestamp returns a timestamp which is greater than start. */
test('NbsDecoder.nextTimestamp() returns a timestamp greater than input', () => {
  const [start, end] = decoder.getTimestampRange({ type: pingType, subtype: 0 });
  const singleTypeTimestamp = decoder.nextTimestamp(start, { type: pingType, subtype: 0} , 1);
  const multiTypeTimestamp = decoder.nextTimestamp(start, testArray , 1);

  assert.ok(
   tsToBigInt(singleTypeTimestamp) > tsToBigInt(start),
   'The new return Nbs timestamp is greater than the input timestamp'
 );

 assert.ok(
  tsToBigInt(multiTypeTimestamp) > tsToBigInt(start),
  'The new return Nbs timestamp is greater than the input timestamp'
);
 })
// });

/* Check stepping forward by five returns the expected timestamp.  */
test(`NbsDecoder.nextTimestamp() returns exact value expected`, () => {
  const [start, end] = decoder.getTimestampRange( { type: pingType, subtype: 0 });
  const singleTypeTimestamp = decoder.nextTimestamp(start, { type: pingType, subtype: 0 }, 5);
  const multiTypeTimestamp = decoder.nextTimestamp(start, testArray , 5);

  assert.equal(tsToBigInt(singleTypeTimestamp),
  tsToBigInt({seconds: 1015, nanos: 0}))

  assert.equal(tsToBigInt(multiTypeTimestamp),
  tsToBigInt({seconds: 1012, nanos: 0}))
});

/* Check stepping backward returns a timestamp which is less than input. */
test('NbsDecoder.nextTimestamp() returns a timestamp less than input', () => {
  const [start, end] = decoder.getTimestampRange({ type: pingType, subtype: 0 });
  const singleTypeTimestamp = decoder.nextTimestamp(end, { type: pingType, subtype: 0 }, -1)

   const multiTypeTimestamp = decoder.nextTimestamp(end, testArray, -1)

  assert.ok(
   tsToBigInt(singleTypeTimestamp) < tsToBigInt(end),
   'The return Nbs timestamp is less than the input timestamp for a single type'
 );

 assert.equal(
  tsToBigInt(singleTypeTimestamp), tsToBigInt({seconds: 1894, nanos: 0}));
  'The return Nbs timestamp exactly matches the expected timestamp'


  assert.ok(
    tsToBigInt(multiTypeTimestamp) < tsToBigInt(end),
    'The return Nbs timestamp is less than the input timestamp for multiple types'
  );
});

/* Error checking. */
test('NbsDecoder.nextTimestamp() Throws error on missing or incorrect inputs.', () => {
  assert.throws(
    () => {
      decoder.nextTimestamp();
    },
    /invalid type for argument `timestamp`: expected positive number or BigInt/,
    'NbsDecoder.nextTimestamp() throws for missing `timestamp` argument'
  );

  assert.throws(
    () => {
      decoder.nextTimestamp("(1500 * 1e9)");
    },
    /invalid type for argument `timestamp`: expected positive number or BigInt/,
    'NbsDecoder.nextTimestamp() throws for invalid `timestamp` argument'
  );

  assert.throws(
    () => {
      decoder.nextTimestamp(0, { type: pingType, subtype: "0" }, 1);
    },
    /invalid type for argument `typeSubtype`: invalid `.subtype`: expected number/,
    'NbsDecoder.nextTimestamp() throws for object with invalid `subtype`'
  );

  assert.throws(
    () => {
      decoder.nextTimestamp((1500 * 1e9), "a");
    },
    /invalid step input. A number was expected./,
    'NbsDecoder.nextTimestamp() throws for missing `timestamp` argument'
  );

})

test.run();
