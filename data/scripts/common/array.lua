-- Copyright(c) 2022-2023, KaoruXun All rights reserved.

local table, ipairs = table, ipairs

local js_array = {}

function js_array.push(tbl, val)
    table.insert(tbl, val)
    return tbl
end

function js_array.pop(tbl)
    return table.remove(tbl, #tbl)
end

function js_array.shift(tbl)
    return table.remove(tbl, 1)
end

function js_array.unshift(tbl, val)
    table.insert(tbl, 1, val)
    return tbl
end

function js_array.reverse(tbl)
    local results = {}
    for _, val in ipairs(tbl) do
        table.insert(results, 1, val)
    end
    return results
end

function js_array.join(tbl, separator)
    separator = separator or ''
    return table.concat(tbl, separator)
end

function js_array.split(str, separator)
    local results = {}
    local i = 1
    separator = separator or '%s'
    for val in str:gmatch('([^' .. separator .. ']+)') do
        results[i] = val
        i = i + 1
    end
    return results
end

function js_array.slice(tbl, start, stop)
    local results = {}
    local length  = #tbl
    start         = start or 1
    stop          = stop or length
    for i = start, stop do
        table.insert(results, tbl[i])
    end
    return results
end

local js = {}
js.array = js_array

return js
