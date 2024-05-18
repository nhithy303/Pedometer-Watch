const Device = require('../models/Device')
const DailyIndicator = require('../models/DailyIndicator')
const UtilityParam = require('../models/UtilityParam')
const {
  multipleMongooseToObject,
  mongooseToObject,
} = require('../../utils/mongoose')

class AdminController {
  // [GET] /admin
  index(req, res, next) {
    if (req.session.admin) {
      res.redirect('/admin/devices')
    } else {
      res.redirect('/admin/signin')
    }
  }

  // [GET] /admin/devices
  devices(req, res, next) {
    if (req.session.admin) {
      Device.find({})
        .then((devices) => {
          res.render('admin/devices', {
            layout: 'admin',
            devices: multipleMongooseToObject(devices),
          })
        })
        .catch(next)
    } else {
      res.redirect('/admin/signin')
    }
  }

  // [GET] /admin/signin
  signin(req, res, next) {
    if (!req.session.admin) {
      res.render('admin/signin', {
        layout: 'admin',
        signin: true,
      })
    } else {
      res.redirect('/admin')
    }
  }

  // [POST] /admin/signin
  psignin(req, res, next) {
    if (req.body.username === 'admin' && req.body.password === 'iotpedometer') {
      delete req.session.username
      req.session.admin = 'admin'
      res.redirect('/admin')
    } else {
      res.render('admin/signin', {
        layout: 'admin',
        signin: true,
        failed: true,
      })
    }
  }

  // [GET] /admin/signout
  signout(req, res, next) {
    delete req.session.admin
    res.redirect('/admin/signin')
  }
}

module.exports = new AdminController()
