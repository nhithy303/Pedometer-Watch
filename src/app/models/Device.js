const mongoose = require('mongoose')

const Schema = mongoose.Schema

const Device = new Schema({
  macaddress: { type: String, default: '' },
  username: { type: String, default: '' },
  password: { type: String, default: '' },
  lastactive: { type: Date, default: Date.now },
})

module.exports = mongoose.model('Device', Device)
