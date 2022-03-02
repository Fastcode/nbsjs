const bindings = require('bindings');

const binding = bindings({
  bindings: 'nbs_decoder',
});

module.exports.NbsDecoder = binding.Decoder;
