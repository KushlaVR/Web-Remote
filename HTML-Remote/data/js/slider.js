var __extends = (this && this.__extends) || (function () {
    var extendStatics = function (d, b) {
        extendStatics = Object.setPrototypeOf ||
            ({ __proto__: [] } instanceof Array && function (d, b) { d.__proto__ = b; }) ||
            function (d, b) { for (var p in b) if (Object.prototype.hasOwnProperty.call(b, p)) d[p] = b[p]; };
        return extendStatics(d, b);
    };
    return function (d, b) {
        if (typeof b !== "function" && b !== null)
            throw new TypeError("Class extends value " + String(b) + " is not a constructor or null");
        extendStatics(d, b);
        function __() { this.constructor = d; }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
})();
var WorkSpace = (function () {
    function WorkSpace(form) {
        var _this = this;
        this.inputs = new Array();
        this.outputs = new Array();
        this.values = new Dictionary();
        this.sent = new Dictionary();
        this.tranCount = 0;
        this.timer = 0;
        this.reportInterval = 100;
        this.readonlyFields = new Array();
        this.tran = 0;
        this._readyToSend = true;
        this.form = form;
        window.addEventListener('resize', function (event) { return _this.UpdateLayout(); }, false);
    }
    WorkSpace.init = function (form) {
        var workSpace = new WorkSpace((form[0]));
        workSpace.registerInputs();
        workSpace.registerOutputs();
        workSpace.ConnectAPI();
    };
    WorkSpace.prototype.ConnectAPI = function () {
        var _this = this;
        $.get("/api/EventSourceName")
            .done(function (EventSourceName) {
            if (window.EventSource) {
                var s = EventSourceName;
                var json = decodeURI(s.substring(s.indexOf("?") + 1)).replace("%3a", ":");
                var parcel = JSON.parse(json);
                _this.client = parcel.client;
                _this.eventSource = new EventSource(EventSourceName);
                _this.eventSource.onopen = function (ev) {
                    _this.setFormat();
                    _this.sendData();
                };
                _this.eventSource.onmessage = function (msg) {
                    $("#message").text(msg.data);
                    _this.receiveData(msg);
                };
                _this.eventSource.onerror = function (event) {
                    $("#message").text("Error...");
                };
            }
        })
            .fail(function () {
            $("#message").text("error");
        });
    };
    WorkSpace.prototype.setFormat = function () {
        var _this = this;
        this.fields = new Array();
        $.each((this.values), function (name, value) {
            _this.fields.push(name);
        });
        this.send(JSON.stringify({ client: this.client, fields: this.fields, readonlyFields: this.readonlyFields }));
    };
    WorkSpace.prototype.readyToSend = function () {
        if (this.eventSource)
            return this._readyToSend;
        return false;
    };
    WorkSpace.prototype.send = function (value) {
        var _this = this;
        if (this.eventSource) {
            this._readyToSend = false;
            $.ajax({
                url: "/api/post",
                data: value,
                cache: false,
                type: 'POST',
                dataType: "json",
                contentType: 'application/json; charset=utf-8'
            }).done(function () {
                console.log("done!");
                _this._readyToSend = true;
            }).fail(function () {
                console.log("fail!");
                _this._readyToSend = true;
            });
        }
    };
    WorkSpace.prototype.sendData = function () {
        var _this = this;
        if (this.timer === 0) {
            if (this.readyToSend() == true) {
                var v = new Array();
                var changed = false;
                for (var i = 0; i < this.fields.length; i++) {
                    var key = this.fields[i];
                    var value = this.values[key];
                    v.push(value);
                    if (this.sent[key] !== this.values[key])
                        changed = true;
                    this.sent[key] = value;
                }
                if (changed == true) {
                    this.tran += 1;
                    this.send(JSON.stringify({ client: this.client, tran: this.tran.toString(), values: v }));
                }
            }
            this.timer = setTimeout(function () {
                _this.timer = 0;
                _this.sendData();
            }, this.reportInterval);
        }
    };
    WorkSpace.prototype.receiveData = function (msg) {
        if (msg.data) {
            var parcel = JSON.parse(msg.data);
            if (this.tran < parcel.tran) {
                this.tran = parseInt(parcel.tran, 10);
                for (var i = 0; i < this.fields.length; i++) {
                    var key = this.fields[i];
                    var val = parcel.values[i];
                    if (this.sent[key] != val) {
                        this.sent[key] = val;
                        this.values[key] = val;
                        this.refreshInput(key, val);
                    }
                }
            }
            else {
                for (var i = 0; i < this.fields.length; i++) {
                    var key = this.fields[i];
                    var val = parcel.values[i];
                    for (var rIndex = 0; rIndex < this.readonlyFields.length; rIndex++) {
                        if (key == this.readonlyFields[rIndex]) {
                            this.sent[key] = val;
                            this.values[key] = val;
                            break;
                        }
                    }
                }
            }
            this.refreshOutput();
        }
    };
    WorkSpace.toggleFullScreen = function () {
        var doc = window.document;
        var docEl = doc.documentElement;
        var requestFullScreen = docEl.requestFullscreen || docEl.mozRequestFullScreen || docEl.webkitRequestFullScreen || docEl.msRequestFullscreen;
        var cancelFullScreen = doc.exitFullscreen || doc.mozCancelFullScreen || doc.webkitExitFullscreen || doc.msExitFullscreen;
        if (!doc.fullscreenElement && !doc.mozFullScreenElement && !doc.webkitFullscreenElement && !doc.msFullscreenElement) {
            requestFullScreen.call(docEl);
        }
        else {
            cancelFullScreen.call(doc);
        }
    };
    WorkSpace.prototype.UpdateLayout = function () {
        for (var i = 0; i < this.inputs.length; i++) {
            this.inputs[i].initLayout();
        }
        for (var o = 0; o < this.outputs.length; o++) {
            this.outputs[o].initLayout();
        }
    };
    WorkSpace.prototype.registerInputs = function () {
        var _this = this;
        var inputs = $(".input", this.form);
        inputs.each(function (index, val) {
            var element = val;
            var input;
            if ($(element).hasClass("slider")) {
                input = new Slider(element);
            }
            else if ($(element).hasClass("btn")) {
                input = new Button(element);
            }
            else {
                input = new Input(element);
            }
            _this.addInput(input);
        });
    };
    WorkSpace.prototype.addInput = function (input) {
        input.workSpace = this;
        this.inputs.push(input);
        input.saveValue();
    };
    WorkSpace.prototype.registerOutputs = function () {
        var _this = this;
        var outputs = $(".output", this.form);
        outputs.each(function (index, val) {
            var output = null;
            output = new Output(val);
            if (!(_this.values[output.name] != undefined)) {
                _this.readonlyFields.push(output.name);
                _this.values[output.name] = 0;
            }
            _this.addOutput(output);
        });
    };
    WorkSpace.prototype.addOutput = function (output) {
        output.workSpace = this;
        this.outputs.push(output);
        output.loadValue();
    };
    WorkSpace.prototype.beginTransaction = function () {
        this.tranCount += 1;
    };
    WorkSpace.prototype.endTransaction = function () {
        this.tranCount -= 1;
        if (this.tranCount === 0) {
            for (var i = 0; i < this.outputs.length; i++) {
                this.outputs[i].loadValue();
            }
        }
    };
    WorkSpace.prototype.refreshInput = function (key, value) {
        for (var i = 0; i < this.inputs.length; i++) {
            this.inputs[i].loadValue(key, value);
        }
    };
    WorkSpace.prototype.refreshOutput = function () {
        for (var i = 0; i < this.outputs.length; i++) {
            this.outputs[i].loadValue();
        }
    };
    return WorkSpace;
}());
var Dictionary = (function () {
    function Dictionary(init) {
        if (init) {
            for (var x = 0; x < init.length; x++) {
                this[init[x].key] = init[x].value;
            }
        }
    }
    return Dictionary;
}());
var Point = (function () {
    function Point() {
        this.x = 0;
        this.y = 0;
    }
    return Point;
}());
var Input = (function () {
    function Input(element) {
        this.element = element;
        this.jElement = $(element);
        this.name = this.jElement.attr("name");
    }
    Input.prototype.saveValue = function () {
        if (!this.workSpace)
            return;
        this.workSpace.beginTransaction();
        var val = this.jElement.attr("value");
        if (val) {
            this.workSpace.values[this.name] = val;
        }
        this.workSpace.endTransaction();
    };
    Input.prototype.loadValue = function (key, value) {
        if (key == this.name) {
            this.jElement.attr("value", value);
        }
    };
    Input.prototype.initLayout = function () {
    };
    return Input;
}());
var Slider = (function (_super) {
    __extends(Slider, _super);
    function Slider(element) {
        var _this = _super.call(this, element) || this;
        _this.pressed = false;
        _this.handlePos = new Point();
        _this.value = new Point();
        _this.center = new Point();
        _this.autoCenterX = false;
        _this.autoCenterY = false;
        _this.handle = $(".handle", element)[0];
        var pot = $(".pot", element);
        if (pot.length > 0) {
            _this.pot = pot[0];
        }
        if ("ontouchstart" in document.documentElement) {
            _this.element.addEventListener('touchstart', function (event) { return _this.onTouchStart(event); }, false);
            _this.element.addEventListener('touchmove', function (event) { return _this.onTouchMove(event); }, false);
            _this.element.addEventListener('touchend', function (event) { return _this.onTouchEnd(event); }, false);
        }
        else {
            _this.element.addEventListener('mousedown', function (event) { return _this.onMouseDown(event); }, false);
            _this.element.addEventListener('mousemove', function (event) { return _this.onMouseMove(event); }, false);
            _this.element.addEventListener('mouseup', function (event) { return _this.onMouseUp(event); }, false);
        }
        _this.initLayout();
        if ($(element).data("center")) {
            _this.autoCenterX = true;
            _this.autoCenterY = true;
        }
        else if ($(element).data("center-x")) {
            _this.autoCenterX = true;
        }
        else if ($(element).data("center-y")) {
            _this.autoCenterY = true;
        }
        _this.refreshLayout(true);
        return _this;
    }
    Slider.prototype.onTouchStart = function (event) {
        this.pressed = true;
        this.element.style.zIndex = "100";
    };
    Slider.prototype.onTouchMove = function (event) {
        event.preventDefault();
        if (this.pressed === true) {
            this.handlePos = Slider.pointFromTouch(this.element, event.targetTouches[0]);
            this.refreshLayout(false);
            this.saveValue();
        }
    };
    Slider.prototype.onTouchEnd = function (event) {
        this.pressed = false;
        if (this.autoCenterX)
            this.handlePos.x = this.center.x;
        if (this.autoCenterY)
            this.handlePos.y = this.center.y;
        this.refreshLayout(true);
        this.saveValue();
        this.element.style.zIndex = "0";
    };
    Slider.prototype.onMouseDown = function (event) {
        this.pressed = true;
        this.element.style.zIndex = "100";
    };
    Slider.prototype.onMouseMove = function (event) {
        if (this.pressed === true) {
            this.handlePos = Slider.pointFromMouseEvent(this.element, event);
            this.refreshLayout(false);
            this.saveValue();
        }
    };
    Slider.prototype.onMouseUp = function (event) {
        this.pressed = false;
        if (this.autoCenterX)
            this.handlePos.x = this.center.x;
        if (this.autoCenterY)
            this.handlePos.y = this.center.y;
        this.refreshLayout(true);
        this.saveValue();
        this.element.style.zIndex = "0";
    };
    Slider.prototype.refreshLayout = function (clip) {
        if (clip) {
            if (this.handlePos.x < 0)
                this.handlePos.x = 0;
            if (this.handlePos.y < 0)
                this.handlePos.y = 0;
            if (this.handlePos.x > this.element.clientWidth)
                this.handlePos.x = this.element.clientWidth;
            if (this.handlePos.y > this.element.clientHeight)
                this.handlePos.y = this.element.clientHeight;
        }
        this.handle.style.left = '' + (this.handlePos.x - (this.handle.clientWidth / 2)) + 'px';
        this.handle.style.top = '' + (this.handlePos.y - (this.handle.clientHeight / 2)) + 'px';
        var clipped = new Point();
        clipped.x = this.handlePos.x;
        clipped.y = this.handlePos.y;
        if (clipped.x < 0)
            clipped.x = 0;
        if (clipped.y < 0)
            clipped.y = 0;
        if (clipped.x > this.element.clientWidth)
            clipped.x = this.element.clientWidth;
        if (clipped.y > this.element.clientHeight)
            clipped.y = this.element.clientHeight;
        var normalized = new Point();
        normalized.x = (this.center.x - clipped.x) * 100.0 / (this.element.clientWidth / 2.0);
        normalized.y = (this.center.y - clipped.y) * 100.0 / (this.element.clientHeight / 2.0);
        this.value = normalized;
        if (this.pot) {
            this.pot.style.left = '' + (clipped.x - (this.pot.clientWidth / 2.0)) + 'px';
            this.pot.style.top = '' + (clipped.y - (this.pot.clientHeight / 2.0)) + 'px';
        }
    };
    Slider.prototype.saveValue = function () {
        if (!this.workSpace)
            return;
        this.workSpace.beginTransaction();
        var key_x = this.name + "_x";
        this.workSpace.values[key_x] = Slider.numToString(this.value.x);
        var key_y = this.name + "_y";
        this.workSpace.values[key_y] = Slider.numToString(this.value.y);
        this.workSpace.endTransaction();
    };
    Slider.prototype.loadValue = function (key, value) {
        if (this.pressed == true)
            return;
        var key_x = this.name + "_x";
        var key_y = this.name + "_y";
        var refresh = false;
        if (key == key_x) {
            this.value.x = value;
            refresh = true;
        }
        if (key == key_y) {
            this.value.y = value;
            refresh = true;
        }
        if (refresh == true) {
            this.initLayout();
        }
    };
    Slider.prototype.initLayout = function () {
        this.center.x = this.element.clientWidth / 2;
        this.center.y = this.element.clientHeight / 2;
        var x = this.element.clientWidth / 2.0;
        var y = this.element.clientHeight / 2.0;
        this.handlePos.x = this.center.x - this.value.x * x / 100.0;
        this.handlePos.y = this.center.y - this.value.y * y / 100.0;
        this.handle.style.left = '' + (this.handlePos.x - (this.handle.clientWidth / 2)) + 'px';
        this.handle.style.top = '' + (this.handlePos.y - (this.handle.clientHeight / 2)) + 'px';
        if (this.pot) {
            this.pot.style.left = '' + (this.handlePos.x - (this.pot.clientWidth / 2.0)) + 'px';
            this.pot.style.top = '' + (this.handlePos.y - (this.pot.clientHeight / 2.0)) + 'px';
        }
    };
    Slider.numToString = function (n) {
        return (Math.round(n * 100.0) / 100.0).toString(10);
    };
    Slider.pointFromMouseEvent = function (container, e) {
        var m_posx = 0, m_posy = 0, e_posx = 0, e_posy = 0;
        if (!e) {
            e = window.event;
        }
        if (e.pageX || e.pageY) {
            m_posx = e.pageX;
            m_posy = e.pageY;
        }
        else if (e.clientX || e.clientY) {
            m_posx = e.clientX + document.body.scrollLeft
                + document.documentElement.scrollLeft;
            m_posy = e.clientY + document.body.scrollTop
                + document.documentElement.scrollTop;
        }
        if (container.offsetParent) {
            do {
                e_posx += container.offsetLeft;
                e_posy += container.offsetTop;
            } while (container = container.offsetParent);
        }
        var pt = new Point();
        pt.x = (m_posx - e_posx);
        pt.y = (m_posy - e_posy);
        return pt;
    };
    Slider.pointFromTouch = function (container, e) {
        var m_posx = 0, m_posy = 0, e_posx = 0, e_posy = 0;
        if (e.pageX || e.pageY) {
            m_posx = e.pageX;
            m_posy = e.pageY;
        }
        else if (e.clientX || e.clientY) {
            m_posx = e.clientX + document.body.scrollLeft
                + document.documentElement.scrollLeft;
            m_posy = e.clientY + document.body.scrollTop
                + document.documentElement.scrollTop;
        }
        if (container.offsetParent) {
            do {
                e_posx += container.offsetLeft;
                e_posy += container.offsetTop;
            } while (container = container.offsetParent);
        }
        var pt = new Point();
        pt.x = (m_posx - e_posx);
        pt.y = (m_posy - e_posy);
        return pt;
    };
    return Slider;
}(Input));
var Button = (function (_super) {
    __extends(Button, _super);
    function Button(element) {
        var _this = _super.call(this, element) || this;
        _this.audio = null;
        var sound = _this.jElement.data("sound");
        if (sound) {
            _this.audio = new Audio(sound);
            _this.audio.load();
        }
        _this.sound_duration = _this.jElement.data("sound-duration");
        if ("ontouchstart" in document.documentElement) {
            _this.element.addEventListener('touchstart', function (event) { return _this.onTouchStart(event); }, false);
            _this.element.addEventListener('touchend', function (event) { return _this.onTouchEnd(event); }, false);
        }
        else {
            _this.element.addEventListener('mousedown', function (event) { return _this.onMouseDown(event); }, false);
            _this.element.addEventListener('mouseup', function (event) { return _this.onMouseUp(event); }, false);
        }
        return _this;
    }
    Button.prototype.onTouchStart = function (event) {
        this.pressed = true;
        this.saveValue();
        this.Activate();
        this.playSound();
        event.preventDefault();
    };
    Button.prototype.onTouchEnd = function (event) {
        this.pressed = false;
        this.saveValue();
        event.preventDefault();
    };
    Button.prototype.onMouseDown = function (event) {
        this.pressed = true;
        this.saveValue();
        this.Activate();
        this.playSound();
        event.preventDefault();
    };
    Button.prototype.onMouseUp = function (event) {
        this.pressed = false;
        this.saveValue();
        event.preventDefault();
    };
    Button.prototype.Activate = function () {
        var _this = this;
        this.jElement.addClass("active");
        setTimeout(function () { _this.jElement.removeClass("active"); }, 200);
    };
    Button.prototype.playSound = function () {
        var _this = this;
        if (this.audio == null)
            return;
        if (!this.audio.paused)
            return;
        this.audio.currentTime = 0;
        var playPromise = this.audio.play();
        if (playPromise !== undefined) {
            playPromise.then(function (_) {
                setTimeout(function () { _this.audio.pause(); }, _this.sound_duration);
            });
        }
    };
    Button.prototype.saveValue = function () {
        if (!this.workSpace)
            return;
        this.workSpace.beginTransaction();
        var key = this.name;
        if (this.pressed) {
            this.workSpace.values[key] = "1";
        }
        else {
            this.workSpace.values[key] = "0";
        }
        this.workSpace.endTransaction();
    };
    return Button;
}(Input));
var Output = (function () {
    function Output(element) {
        this.audio = null;
        this.element = element;
        this.jElement = $(element);
        this.name = this.jElement.data("input");
        var sound = this.jElement.data("sound");
        if (sound) {
            this.audio = new Audio(sound);
            this.audio.load();
            this.sound_duration = this.jElement.data("sound-duration");
        }
    }
    Output.prototype.loadValue = function () {
        if (!(this.workSpace.values[this.name] == undefined)) {
            var newValue = this.workSpace.values[this.name];
            if (this.element.tagName.toUpperCase() == "INPUT") {
                this.jElement.val(newValue);
            }
            if (this.element.tagName.toUpperCase() == "IMG") {
                if (newValue == "0") {
                    this.jElement.addClass("hidden");
                }
                else {
                    this.jElement.removeClass("hidden");
                }
            }
            if (this.element.classList.contains("progress-bar")) {
                this.jElement.width((newValue) + "%");
            }
            else {
                this.jElement.text(newValue);
            }
            if (this.value == "0" && !(newValue == "0")) {
                this.playSound();
            }
            ;
            this.value = newValue;
        }
    };
    Output.prototype.initLayout = function () {
    };
    Output.prototype.playSound = function () {
        var _this = this;
        if (this.audio == null)
            return;
        if (!this.audio.paused)
            return;
        this.audio.currentTime = 0;
        var playPromise = this.audio.play();
        if (playPromise !== undefined && this.sound_duration !== undefined) {
            playPromise.then(function (_) {
                setTimeout(function () { _this.audio.pause(); }, _this.sound_duration);
            });
        }
    };
    return Output;
}());
//# sourceMappingURL=slider.js.map