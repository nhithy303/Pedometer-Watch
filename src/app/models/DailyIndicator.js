const mongoose = require('mongoose')

const Schema = mongoose.Schema

const DailyIndicator = new Schema({
  deviceuid: { type: String, default: '' },
  date: { type: String, default: '' },
  totalsteps: { type: Number, default: 0 },
  totaltime: { type: Number, default: 0 },
  rounds: [
    {
      start: { type: String, default: '' },
      end: { type: String, default: '' },
      steps: { type: Number, default: 0 },
    },
  ],
})

module.exports = mongoose.model('DailyIndicator', DailyIndicator)
