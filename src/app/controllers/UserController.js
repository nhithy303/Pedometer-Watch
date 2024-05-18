const Device = require('../models/Device')
const DailyIndicator = require('../models/DailyIndicator')
const {
  multipleMongooseToObject,
  mongooseToObject,
} = require('../../utils/mongoose')

class UserController {
  index(req, res) {
    if (req.session.username) {
      res.render('user/landing', {
        signedin: true,
      })
    } else {
      res.render('user/landing')
    }
  }

  // [GET] /dashboard
  dashboard(req, res, next) {
    if (req.session.username) {
      DailyIndicator.find({ deviceuid: req.session.deviceuid })
        .then((indicators) => {
          const today = new Date()
          let avgSteps = 0
          let maxSteps = 0
          let minSteps = 99999999
          let todayIndicator = null

          indicators.forEach((item) => {
            // get today indicator
            let itemDate = item.date.split('-').join('/')
            if (itemDate === today.toLocaleDateString('en-GB')) {
              todayIndicator = item
            }

            // calculate average steps per day
            avgSteps += item.totalsteps
            // find max steps
            if (item.totalsteps >= maxSteps) {
              maxSteps = item.totalsteps
            }
            // find min steps
            if (item.totalsteps <= minSteps) {
              minSteps = item.totalsteps
            }
          })
          avgSteps = Math.ceil(avgSteps / indicators.length)

          res.render('user/dashboard', {
            signedin: true,
            today: new Date(),
            avgSteps: avgSteps,
            maxSteps: maxSteps,
            minSteps: minSteps,
            indicators: multipleMongooseToObject(indicators),
            todayIndicator: mongooseToObject(todayIndicator),
          })
        })
        .catch(next)
    } else {
      res.redirect('/signin')
    }
  }

  // [GET] /signin
  signin(req, res, next) {
    if (!req.session.username) {
      res.render('user/signin', {
        title: 'Đăng nhập |',
      })
    } else {
      res.redirect('/dashboard')
    }
  }

  // [POST] /signin
  psignin(req, res, next) {
    Device.findOne({ username: req.body.username })
      .then((device) => {
        if (device) {
          if (device.password === req.body.password) {
            req.session.username = device.username
            req.session.deviceuid = device.macaddress
            res.redirect('/dashboard')
          } else {
            res.render('user/signin', {
              title: 'Đăng nhập |',
              failed: true,
            })
          }
        } else {
          res.render('user/signin', {
            title: 'Đăng nhập |',
            failed: true,
          })
        }
      })
      .catch(next)
  }

  // [GET] /signout
  signout(req, res, next) {
    req.session.destroy()
    res.redirect('/')
  }

  // [PUT] /password/:username
  // changePassword(req, res, next) {
  //   Device.findOne({ username: req.params.username })
  //     .then((device) => {
  //       if (device) {
  //         device.validatePassword(req.body.currentPassword, (err, match) => {
  //           if (match) {
  //             device.password = req.body.newPassword
  //             device
  //               .save()
  //               .then(() => res.redirect('/settings'))
  //               .catch(next)
  //           } else {
  //             res.render('user/student/settings', {
  //               title: 'Cài đặt |',
  //               student: req.session.username,
  //               active: 'settings',
  //               failed: true,
  //             })
  //           }
  //         })
  //       } else {
  //         res.redirect('/settings')
  //       }
  //     })
  //     .catch(next)
  // }
}

module.exports = new UserController()
