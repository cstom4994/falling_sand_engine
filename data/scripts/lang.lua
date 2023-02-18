i18n = require("i18n")
i18n_ex = require("common.i18n")

local lang_pack = {
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
        ui_tweaks = "Tweaks",
        ui_info = "基本信息",
        ui_debug = "调试",
        ui_telemetry = "遥测",
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
        ui_close = "关闭",
        ui_line = "行",
        ui_readonly_mode = "只读模式",
        ui_undo = "撤销",
        ui_redo = "恢复",
        ui_copy = "复制",
        ui_paste = "粘贴",
        ui_cut = "剪切",
        ui_delete = "删除",
        ui_selectall = "全选",
        ui_view = "查看",
        ui_debug_materials = "材质",
        ui_debug_items = "物品",
        ui_play = "单人游戏",
        ui_newworld = "新世界",
        ui_return = "返回",
        ui_option = "选项",
        ui_exit = "退出",
        ui_create_world = "创建世界",
        ui_worldname = "世界名",
        ui_worldgenerator = "世界生成器",
        ui_create = "创建",
        ui_file_dialog = "选择文件",
        ui_chunk = "区块",
        ui_frametime = "帧耗时",
        ui_frameinspector = "帧检查器",
        ui_profiler = "性能检查器",
        ui_inspecting = "监视",
        ui_entities = "实体",
        ui_scene = "场景",
        ui_system = "系统",
        ui_cvars = "CVars"
    }
}

i18n.load(lang_pack)

function translate(str) return i18n.translate(str) end

function setlocale(loc)
    i18n.setLocale(loc)
    i18n_ex:set_namespace(loc)
    METADOT_INFO("Using Language " .. loc)
end
