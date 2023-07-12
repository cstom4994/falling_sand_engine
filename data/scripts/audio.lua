-- Copyright(c) 2022-2023, KaoruXun All rights reserved.

InitAudio = function()


    audio_init()

    -- typedef unsigned int FMOD_STUDIO_LOAD_BANK_FLAGS;
    -- #define FMOD_STUDIO_LOAD_BANK_NORMAL                        0x00000000
    -- #define FMOD_STUDIO_LOAD_BANK_NONBLOCKING                   0x00000001
    -- #define FMOD_STUDIO_LOAD_BANK_DECOMPRESS_SAMPLES            0x00000002
    -- #define FMOD_STUDIO_LOAD_BANK_UNENCRYPTED                   0x00000004

    audio_load_bank("data/assets/audio/fmod/Build/Desktop/Master.bank", 0)
    audio_load_bank("data/assets/audio/fmod/Build/Desktop/Master.strings.bank", 0)

    local audio_event = {
        "event:/Music/Background1",
        "event:/Music/Title",
        "event:/Player/Jump",
        "event:/Player/Fly",
        "event:/Player/Wind",
        "event:/Player/Impact",
        "event:/World/Sand",
        "event:/World/WaterFlow",
        "event:/GUI/GUI_Hover",
        "event:/GUI/GUI_Slider"
    }

    for k, v in ipairs(audio_event) do
        audio_load_event(v)
    end

    audio_play_event("event:/Player/Fly")
    audio_play_event("event:/Player/Wind")
    audio_play_event("event:/World/Sand")
    audio_play_event("event:/World/WaterFlow")

end

EndAudio = function ()
    
end