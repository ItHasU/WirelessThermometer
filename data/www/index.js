//-- Init ---------------------------------------------------------------------

var $components = {
    ssidSelect: $("#ssid"),
    ssidRefreshButton: $("ssid_refresh"),
    ssidOpenText: $("#ssid_open"),
    passwordInput: $("#password")
}

function init() {
    //-- Register select callback --
    $components.ssidSelect.change(() => {
        var $selected = $components.ssidSelect.children("option:selected");
        var network = $selected.data("network");
        if (network.secure) {
            $components.passwordInput.parent().show();
            $components.ssidOpenText.hide();
        } else {
            $components.ssidOpenText.show();
            $components.passwordInput.parent().hide();
            $components.passwordInput.val("");
        }
    });
    $components.ssidRefreshButton.click(() => {
        getInfo();
    });

    //-- Register save --

    //-- Register reboot --

}


//-- Progress bar -------------------------------------------------------------

function showProgress() {
    $("#progress").show();
}

function hideProgress() {
    $("#progress").hide();
}

function errorMessage(text) {
    if (text) {
        $("#error").html(text).show();
    } else {
        $("#error").hide();
    }
}

//-- Form ---------------------------------------------------------------------

function fillForm(infos) {
    $("#chip_id").text(infos.chip_id);

    let $ssidSelect = $("#ssid");
    for (let network of infos.networks) {
        $(`<option>${network.ssid}</option>`).data("network", network).appendTo($ssidSelect);
    }
}

//-- AJAX ---------------------------------------------------------------------

/** Retrieve information from the chip (id, wireless access points, ...) */
function getInfo() {
    showProgress();
    $.getJSON("/scan.json").done((infos) => {
        fillForm(infos);
    }).fail(() => {
        errorMessage("Failed to load chip info");
    }).always(hideProgress);
}

//-- Main ---------------------------------------------------------------------

$(() => {
    init();
    getInfo();
});