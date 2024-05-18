module.exports = {
  increaseIndex: (index) => {
    return index + 1
  },
  formatDate: (date) => {
    return date.toLocaleDateString('en-GB')
  },
  formatDateTime: (date) => {
    return date.toLocaleString('en-GB')
  },
  shortDate: (date) => {
    return date.toLocaleDateString('fr-CA')
  },
  detailDate: (date) => {
    let fullDate = date.toDateString()
    return fullDate.slice(0, 3) + ',' + fullDate.slice(3, fullDate.length)
  },
}
