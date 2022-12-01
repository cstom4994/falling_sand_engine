i18n = require("i18n")

i18n.load({
    en = {
        welcome = "Welcome to MetaDot",
        loaded_vec = "loaded vec lib",
        age_msg = "your age is %{age}.",
        phone_msg = {
            one = "you have one new message.",
            other = "you have %{count} new messages."
        }
    },
    zh = {
        welcome = "欢迎来到 MetaDot",
        ui_tweaks = "MetaEngine Tweaks",
        ui_info = "基本信息",
        ui_test = "测试"
    }
})

i18n.setLocale('zh')

function translate(str)
    return i18n.translate(str)
end
