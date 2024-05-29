const mongoose = require('mongoose')
const secret = require('./secret')
const connectionStr = secret.connectionStr

async function connect() {
  try {
    await mongoose.connect(connectionStr)
    console.log('Connect successfully!')
  } catch (error) {
    console.log(`Connect failure!\n=> Error: ${error}`)
  }
}

module.exports = { connect }
