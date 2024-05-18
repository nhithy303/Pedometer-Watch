const express = require('express')
const router = express.Router()

const adminController = require('../app/controllers/AdminController')

router.get('/devices', adminController.devices)
router.post('/signin', adminController.psignin)
router.get('/signin', adminController.signin)
router.get('/signout', adminController.signout)
router.get('/', adminController.index)

module.exports = router
