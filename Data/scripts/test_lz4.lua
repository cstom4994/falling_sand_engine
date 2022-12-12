local function test_frame(s)
    local e = _metadot_lz4.compress(s)
    local d = _metadot_lz4.decompress(e)
    assert(s == d)
    print(#e .. '/' .. #d)
end

test_frame(string.rep("0123456789", 100000))
