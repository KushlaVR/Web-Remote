/**
 * 
 * */
class KVR {
    static load() {
        var form = $("#form");
        $.get("/api/setup")
            .done(function (data) {
                $.each(data, (name: string, value: string) => {
                    console.log("Name: " + name + ", Value: " + value);
                    $("[name=" + name + "]", form).val(value);
                });
            })
            .fail(function () {
                console.log("error");
            });
    }
}

