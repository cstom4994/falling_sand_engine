function RandomWithWeight(t, weights)
    local sum = 0
    for i = 1, #weights do
        sum = sum + weights[i]
    end
    local compareWeight = math.random(1, sum)
    local weightIndex = 1
    while sum > 0 do
        sum = sum - weights[weightIndex]
        if sum < compareWeight then
        return t[weightIndex], weightIndex
        end
        weightIndex = weightIndex + 1
    end
    return nil, nil
end