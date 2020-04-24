var WorkSpace = (function () {
    function WorkSpace() {
        this.value = 0;
    }
    WorkSpace.init = function (form) {
        WorkSpace.form = (form[0]);
        WorkSpace.form.oninput = function () {
            WorkSpace.form_input();
        };
        var controls = $(".slider", WorkSpace.form);
        controls.each(function (index, element) {
            var slider = new Slider(element);
        });
    };
    WorkSpace.form_input = function () {
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
    return WorkSpace;
}());
var Point = (function () {
    function Point() {
    }
    return Point;
}());
var Slider = (function () {
    function Slider(element) {
        var _this = this;
        this.pressed = false;
        this.moved = new Point();
        this.center = new Point();
        this.autoCenterX = false;
        this.autoCenterY = false;
        this.element = element;
        this.pin = $(".pin", element)[0];
        if ("ontouchstart" in document.documentElement) {
            this.element.addEventListener('touchstart', function (event) { return _this.onTouchStart(event); }, false);
            this.element.addEventListener('touchmove', function (event) { return _this.onTouchMove(event); }, false);
            this.element.addEventListener('touchend', function (event) { return _this.onTouchEnd(event); }, false);
        }
        else {
            this.element.addEventListener('mousedown', function (event) { return _this.onMouseDown(event); }, false);
            this.element.addEventListener('mousemove', function (event) { return _this.onMouseMove(event); }, false);
            this.element.addEventListener('mouseup', function (event) { return _this.onMouseUp(event); }, false);
        }
        this.center.x = this.element.clientWidth / 2;
        this.center.y = this.element.clientHeight / 2;
        this.moved.x = this.center.x;
        this.moved.y = this.center.y;
        if ($(element).data("center")) {
            this.autoCenterX = true;
            this.autoCenterY = true;
        }
        else if ($(element).data("center-x")) {
            this.autoCenterX = true;
        }
        else if ($(element).data("center-y")) {
            this.autoCenterY = true;
        }
        this.drawInternal(true);
    }
    Slider.prototype.onTouchStart = function (event) {
        this.pressed = true;
    };
    Slider.prototype.onTouchMove = function (event) {
        event.preventDefault();
        if (this.pressed === true) {
            this.moved = Slider.pointFromTouch(this.element, event.targetTouches[0]);
            this.drawInternal(false);
        }
    };
    Slider.prototype.onTouchEnd = function (event) {
        this.pressed = false;
        if (this.autoCenterX)
            this.moved.x = this.center.x;
        if (this.autoCenterY)
            this.moved.y = this.center.y;
        this.drawInternal(true);
    };
    Slider.prototype.onMouseDown = function (event) {
        this.pressed = true;
    };
    Slider.prototype.onMouseMove = function (event) {
        if (this.pressed === true) {
            this.moved = Slider.pointFromMouseEvent(this.element, event);
            this.drawInternal(false);
        }
    };
    Slider.prototype.onMouseUp = function (event) {
        this.pressed = false;
        if (this.autoCenterX)
            this.moved.x = this.center.x;
        if (this.autoCenterY)
            this.moved.y = this.center.y;
        this.drawInternal(true);
    };
    Slider.prototype.drawInternal = function (clip) {
        if (clip) {
            if (this.moved.x < 0)
                this.moved.x = 0;
            if (this.moved.y < 0)
                this.moved.y = 0;
            if (this.moved.x > this.element.clientWidth)
                this.moved.x = this.element.clientWidth;
            if (this.moved.y > this.element.clientHeight)
                this.moved.y = this.element.clientHeight;
        }
        this.pin.style.left = '' + (this.moved.x - (this.pin.clientWidth / 2)) + 'px';
        this.pin.style.top = '' + (this.moved.y - (this.pin.clientHeight / 2)) + 'px';
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
}());
//# sourceMappingURL=slider.js.map