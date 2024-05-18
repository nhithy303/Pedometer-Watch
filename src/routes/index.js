const userRouter = require('./user')
const adminRouter = require('./admin')

const route = (app) => {
  app.use('/', userRouter)
  app.use('/admin', adminRouter)
}

module.exports = route
