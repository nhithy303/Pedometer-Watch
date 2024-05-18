const mongoose = require('mongoose')
const connectionStr =
  'mongodb+srv://funlish:cntt4701104@funlish.ixuxxok.mongodb.net/Pedometer?retryWrites=true&w=majority'

async function connect() {
  try {
    await mongoose.connect(connectionStr)
    console.log('Connect successfully!')
  } catch (error) {
    console.log(`Connect failure!\n=> Error: ${error}`)
  }
}

module.exports = { connect }
