var KVR = (function () {
    function KVR() {
    }
    KVR.load = function () {
        var form = $("#form");
        $.get("/api/setup")
            .done(function (data) {
            $.each(data, function (name, value) {
                console.log("Name: " + name + ", Value: " + value);
                $("[name=" + name + "]", form).val(value);
            });
        })
            .fail(function () {
            console.log("error");
        });
    };
    return KVR;
}());
//# sourceMappingURL=kushlavr.js.map