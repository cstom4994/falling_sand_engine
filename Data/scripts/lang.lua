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
        test = "测试",
        ui_tweaks = "MetaEngine Tweaks",
        ui_info = "基本信息",
        ui_test = "测试",
        ui_console = "控制台",
        ui_settings = "设置",
        ui_color_output = "输出颜色",
        ui_auto_scroll = "自动滚动",
        ui_filter_bar = "过滤栏",
        ui_time_stamps = "时间戳",
        ui_reset_settings = "重置设置",
        ui_reset = "重置",
        ui_cancel = "取消",
        ui_appearance = "外观",
        ui_background = "背景",
        ui_scripts = "脚本",
        ui_scripts_editor = "脚本编辑器",
        ui_file = "文件",
        ui_open = "打开",
        ui_save = "保存",
        ui_edit = "编辑",
        ui_line = "行"
    }
})

function translate(str) return i18n.translate(str) end

function setlocale(loc)
    i18n.setLocale(loc)
    METADOT_INFO("Using Language " .. loc)
end
