class KVR {
    static load() {
        var jqxhr = $.get("/api/setup")
            .done(function (data) {
                $.each(data, (name: string, value: string) => {
                    console.log("Name: " + name + ", Value: " + value);
                    var form = $("#form");
                    $("[name=" + name + "]", form).val(value);
                });
            })
            .fail(function () {
                console.log("error");
            });
    }
}