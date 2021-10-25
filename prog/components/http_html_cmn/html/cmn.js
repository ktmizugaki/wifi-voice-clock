"use strict";
var cmn = {
  EVENT_NAMES: {
    click:true,change:true,keydown:true,keypress:true,keyup:true,submit:true,focus:true
  },
  LAST_REQ: Promise.resolve(null),
  req: function() {
    var args = arguments;
    return this.LAST_REQ = this.LAST_REQ.catch(function(){})
      .then(function() {
        return fetch.apply(null, args);
      });
  },
  api: function(path, params) {
    if (params && params.method !== 'GET') {
      params.headers = Object.assign({'Content-Type':'application/x-www-form-urlencoded'}, params.headers);
    }
    return this.req(location.href+'/'+path, params).then(function(res) {
      return res.json();
    }).catch(function(err) {
      return {status:-1,message:err.message};
    });
  },
  sendfile: function(path, file, type, onprogress) {
    if (!file) {
       return Promise.resolve({status:-1,message:'Invalid arg'});
    }
    /* use XMLHttpRequest to get progress */
    return this.LAST_REQ = this.LAST_REQ.catch(function(){}).then(function() {
      return new Promise(function(resolve, reject) {
        var xhr = new XMLHttpRequest();
        xhr.onload = function() {
          try {
            resolve(JSON.parse(xhr.responseText));
          } catch(e) {
            resolve({status:-1,message:e.message});
          }
        };
        xhr.onerror = function(e) {
          console.log(e);
          resolve({status:-1,message:'network error'});
        };
        xhr.ontimeout = function(e) {
          resolve({status:-1,message:'request timed out'});
        };
        xhr.onabort = function(e) {
          resolve({status:-1,message:'request aborted'});
        };
        if (onprogress) {
          xhr.onprogress = function(ev){ onprogress(ev, false); };
          xhr.upload.onprogress = function(ev){ onprogress(ev, true); };
        }
        xhr.open('POST', location.href+'/'+path);
        xhr.setRequestHeader('Content-Type', type || 'application/octet-stream');
        xhr.send(file);
      });
    });
  },
  ik: function(o,k){return this.hasOwnProperty.call(o,k);},
  ready: function(fn) {
    window.addEventListener('DOMContentLoaded', fn);
  },
  qstr: function(params) {
    var e = encodeURIComponent;
    var qvalues = [];
    for (var i in params) {
      if (this.ik(params, i)) {
        qvalues.push(e(i)+'='+e(params[i]));
      }
    }
    return qvalues.join('&');
  },
  el: function(id) {
    if (id instanceof HTMLElement) return id;
    return document.getElementById(id);
  },
  tag: function(tag, parent) {
    var el = document.createElement(tag);
    if (parent) this.el(parent).appendChild(el);
    return el;
  },
  text: function(string, parent) {
    var el = document.createTextNode(string);
    if (parent) this.el(parent).appendChild(el);
    return el;
  },
  shown: function(el) {
    return !this.el(el).classList.contains('hidden');
  },
  show: function(el, show) {
    this.cls(el, 'hidden', !show);
  },
  cls: function(el, cls, on) {
    el = this.el(el);
    if (on === void 0) on = true;
    if (on) {
      el.classList.add(cls);
    } else {
      el.classList.remove(cls);
    }
  },
  modalShown: function() {
    var els = document.getElementsByClassName('modal');
    for (var i = 0; i < els.length; i++) {
      if (this.shown(els[i])) {
        return true;
      }
    }
    return false;
  },
  modal: function(el, show) {
    if (show === void 0) show = true;
    this.show(el, show);
    this.cls(document.body, 'modal-shown', this.modalShown());
  },
  val: function(form, name, value) {
    var els = this.el(form).elements;
    for (var i = 0; i < els.length; i++) {
      if (els[i].name === name) {
        if (value !== void 0) els[i].value = value;
        return els[i].value;
      }
    }
  },
  chk: function(form, name, value) {
    var els = this.el(form).elements, vals = [], isarr = value instanceof Array;
    for (var i = 0; i < els.length; i++) {
      var el = els[i];
      if (el.name !== name) continue;
      if (value !== void 0) {
        el.checked = isarr? value.indexOf(el.value) !== -1: value==el.value;
      }
      if (el.checked) vals.push(el.value);
    }
    return vals;
  },
  build: function(form, names) {
    var body = {};
    var els = this.el(form).elements;
    for (var i = 0; i < els.length; i++) {
      var el = els[i];
      if ((el.type === 'checkbox' || el.type === 'radio') && !el.checked) continue;
      if (!names || names.indexOf(el.name) > -1) body[el.name] = el.value;
    }
    return body;
  },
  on: function(el, eventName, fn) {
    this.el(el).addEventListener(eventName, fn);
  },
  click: function(el, fn) {
    this.on(el, 'click', function(ev) {
      ev.preventDefault();
      fn.call(this, ev);
    });
  },
  _: 0
};
cmn.tmpl = function(tmpl, parent) {
  if (typeof tmpl !== "object") {
    return this.text(tmpl, parent);
  }
  if (tmpl instanceof HTMLElement) {
    return tmpl;
  }
  var el = this.tag(tmpl[0], parent), at = tmpl[1]
  for (var i in at) {
    if (this.ik(this.EVENT_NAMES, i)) {
        this.on(el, i, at[i]);
    } else if (this.ik(at, i)) {
      el.setAttribute(i, at[i]);
    }
  }
  for (var i = 2; i < tmpl.length; i++) {
    this.tmpl(tmpl[i], el);
  }
  return el;
};
cmn.tmpls = function(tmpls, parent) {
  if (parent) parent.innerHTML = '';
  var els = [];
  for (var i = 0; i < tmpls.length; i++) {
    els.push(this.tmpl(tmpls[i], parent));
  }
  return els;
}
