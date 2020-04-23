var Slider = /** @class */ (function () {
    function Slider(form) {
        this.value = 0;
        var f = form[0];
        this.form = f;
    }
    Slider.prototype.init = function () {
        var _this = this;
        this.form.oninput = function () {
            _this.form_input();
        };
    };
    Slider.prototype.form_input = function () {
    };
    Slider.toggleFullScreen = function () {
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
    return Slider;
}());
//# sourceMappingURL=slider.js.map