"use strict";
var cmn = {
  EVENT_NAMES: {
    click:true,change:true,keydown:true,keypress:true,keyup:true,submit:true
  },
  LAST_REQ: Promise.resolve(null),
  req: function() {
    var args = arguments;
    return this.LAST_REQ = this.LAST_REQ.catch(function(){})
      .then(function() {
        return fetch.apply(null, args);
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
  el: function(id, el) {
    if (id instanceof HTMLElement) return id;
    return (el||document).getElementById(id);
  },
  tag: function(tag, parent) {
    var el = document.createElement(tag);
    if (parent) parent.appendChild(el);
    return el;
  },
  text: function(string, parent) {
    var el = document.createTextNode(string);
    if (parent) this.el(parent).appendChild(el);
    return el;
  },
  shown: function(el) {
    return this.el(el).classList.contains('hidden');
  },
  show: function(el, show) {
    this.cls(el, 'hidden', !show);
  },
  cls: function(el, cls, on) {
    el = this.el(el);
    if (on === void(0)) on = true;
    if (on) {
      el.classList.add(cls);
    } else {
      el.classList.remove(cls);
    }
  },
  modal: function(el, show) {
    if (show === void(0)) show = true;
    this.cls(this.el(el), 'hidden', !show);
    this.cls(document.body, 'modal-shown', document.querySelectorAll('.modal:not(.hidden)').length>0);
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
  build: function(form, names) {
    var body = {};
    var els = this.el(form).elements;
    for (var i = 0; i < els.length; i++) {
      var el = els[i];
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
      fn(ev);
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
