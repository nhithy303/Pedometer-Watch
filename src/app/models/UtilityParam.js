const mongoose = require('mongoose')

const Schema = mongoose.Schema

const UtilityParam = new Schema({
  defaultpassword: { type: String, default: '' },
  idxcount: { type: Number, default: 0 },
  unique: { type: Boolean, default: true },
})

module.exports = mongoose.model('UtilityParam', UtilityParam)
