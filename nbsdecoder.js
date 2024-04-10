const bindings = require('bindings');

const binding = bindings({
  bindings: 'nbsdecoder',
});

module.exports.NbsDecoder = binding.Decoder;
module.exports.NbsEncoder = binding.Encoder;
