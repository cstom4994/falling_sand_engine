local i18n = require("i18n")

i18n.set('en.welcome', 'welcome to this program')
i18n.set('zh.welcome', '你好八嘎')

i18n.load({
    en = {
        loaded_vec = "loaded vec lib",
        age_msg = "your age is %{age}.",
        phone_msg = {
            one = "you have one new message.",
            other = "you have %{count} new messages."
        }
    }
})

i18n.setLocale('en') -- English is the default locale anyway

function translate(str)
    return i18n.translate(str)
end
