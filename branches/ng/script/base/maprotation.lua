--[[

    Base module for server-controlled map rotation
    
    Copyright (C) 2009 Graham Daws
    
    MAINTAINER
        Graham

    CONTRIBUTION HISTORY
        * Randomized map and mode selection - Zombie
        * Small and big maps for each game mode, instead of just for ffa_maps - Zombie
        * Client-side map list useful for #nextmap - Zombie
        
    GUIDELINES
        * Transfer global configuration variables to local scope variables on module initialization (in reload_maprotation)
]]

local gamecount = 0
local maps = {}
local client_maps = {}
local modes = nil
local small_maps = {}
local big_maps = {}
local using_best_map_size
local smallgamesize = 0
local smallteamgamesize = 0
local using_random_mode
local using_random_map
local default_gamemode = "ffa"
local default_game_on_empty
local using_server_maprotation

local gamemodes = {
    [ 1] = "ffa",
    [ 2] = "coop-edit",
    [ 3] = "teamplay",
    [ 4] = "instagib",
    [ 5] = "instagib team",
    [ 6] = "efficiency",
    [ 7] = "efficiency team",
    [ 8] = "tactics",
    [ 9] = "tactics teams",
    [10] = "capture",
    [11] = "regen capture",
    [12] = "ctf",
    [13] = "insta ctf",
    [14] = "protect",
    [15] = "insta protect"
}

function server.reload_maprotation()

    local parse = server.parse_list
    
    maps["ffa"]             = table_unique(parse(server["ffa_maps"]))
    maps["templay"]         = table_unique(parse(server["teamplay_maps"]))
    maps["efficiency"]      = table_unique(parse(server["efficiency_maps"]))
    maps["efficiency team"] = table_unique(parse(server["efficiency team_maps"]))
    maps["tactics"]         = table_unique(parse(server["tactics_maps"]))
    maps["tactics team"]    = table_unique(parse(server["tactics team_maps"]))
    maps["instagib"]        = table_unique(parse(server["instagib_maps"]))
    maps["instagib team"]   = table_unique(parse(server["instagib team_maps"]))
    maps["capture"]         = table_unique(parse(server["capture_maps"]))
    maps["regen capture"]   = table_unique(parse(server["regen capture_maps"]))
    maps["ctf"]             = table_unique(parse(server["ctf_maps"]))
    maps["insta ctf"]       = table_unique(parse(server["insta ctf_maps"]))
    maps["protect"]         = table_unique(parse(server["protect_maps"]))
    maps["insta protect"]   = table_unique(parse(server["insta protect_maps"]))
    
    big_maps["ffa"]         = table_unique(parse(server["big_ffa_maps"]))
    big_maps["efficiency"]  = table_unique(parse(server["big_efficiency_maps"]))
    big_maps["tactics"]     = table_unique(parse(server["big_tactics_maps"]))
    big_maps["instagib"]    = table_unique(parse(server["big_instagib_maps"]))
    
    small_maps["ffa"]             = table_unique(parse(server["small_ffa_maps"]))
    small_maps["teamplay"]        = table_unique(parse(server["small_teamplay_maps"]))
    small_maps["efficiency"]      = table_unique(parse(server["small_efficiency_maps"]))
    small_maps["efficiency team"] = table_unique(parse(server["small_efficiency team_maps"]))
    small_maps["tactics"]         = table_unique(parse(server["small_tactics_maps"]))
    small_maps["tactics team"]    = table_unique(parse(server["small_tactics team_maps"]))
    small_maps["instagib"]        = table_unique(parse(server["small_instagib_maps"]))
    small_maps["instagib team"]   = table_unique(parse(server["small_instagib team_maps"]))
    small_maps["capture"]         = table_unique(parse(server["small_capture_maps"]))
    small_maps["regen capture"]   = table_unique(parse(server["small_regen capture_maps"]))
    small_maps["ctf"]             = table_unique(parse(server["small_ctf_maps"]))
    small_maps["insta ctf"]       = table_unique(parse(server["small_insta ctf_maps"]))
    small_maps["protect"]         = table_unique(parse(server["small_protect_maps"]))
    small_maps["insta protect"]   = table_unique(parse(server["small_insta protect_maps"]))
    
    -- Store a list of client-side map rotations so #nextmap can tell the player what map is next when server map rotation is disabled
    client_maps["ffa"]             = table_unique(parse(server["def_ffa_maps"]))
    client_maps["templay"]         = table_unique(parse(server["def_teamplay_maps"]))
    client_maps["efficiency"]      = table_unique(parse(server["def_efficiency_maps"]))
    client_maps["efficiency team"] = table_unique(parse(server["def_efficiency team_maps"]))
    client_maps["tactics"]         = table_unique(parse(server["def_tactics_maps"]))
    client_maps["tactics team"]    = table_unique(parse(server["def_tactics team_maps"]))
    client_maps["instagib"]        = table_unique(parse(server["def_instagib_maps"]))
    client_maps["instagib team"]   = table_unique(parse(server["def_instagib team_maps"]))
    client_maps["capture"]         = table_unique(parse(server["def_capture_maps"]))
    client_maps["regen capture"]   = table_unique(parse(server["def_regen capture_maps"]))
    client_maps["ctf"]             = table_unique(parse(server["def_ctf_maps"]))
    client_maps["insta ctf"]       = table_unique(parse(server["def_insta ctf_maps"]))
    client_maps["protect"]         = table_unique(parse(server["def_protect_maps"]))
    client_maps["insta protect"]   = table_unique(parse(server["def_insta protect_maps"]))
    
    using_best_map_size = (server.use_best_map_size == 1)
    smallgamesize = server.small_gamesize
    smallteamgamesize = server.small_teamgamesize
    
    using_random_mode = (server.use_server_random_moderotation == 1)
    using_random_map = (server.use_server_random_maprotation == 1)
    
    default_gamemode = server.default_gamemode
    default_game_on_empty = (server.default_game_on_empty == 1)
    
    using_server_maprotation = (server.use_server_maprotation == 1)
end

local function get_maplist(mode)

    local maplist = maps[mode]
    
    if using_best_map_size then
        if server.playercount <= smallgamesize then
            maplist = small_maps[mode] or maplist
        elseif gamemodeinfo.teams then
            maplist = big_maps[mode] or maplist
        end
    end
    
    return maplist
end

local function nextmap(mode, i)

    local maplist = get_maplist(mode)
    
    if not maplist or (#maplist == 0) then
        return nil
    elseif #maplist == 1 then
        return maplist[1]
    end
    
    local chosen_map = maplist[(i % #maplist) + 1]
    
    if chosen_map == server.map then
        return nextmap(mode, i + 1)
    end
    
    return chosen_map
end

function server.is_known_map(mapname, gamemode)

    if not gamemode then
        for k,list in pairs(maps) do
            if server.is_known_map(mapname, k) then return true end
        end
        return false
    end
    
    for i,mapnameX in ipairs(maps[gamemode]) do
        if mapname == mapnameX then return true end
    end
    
    return false
end

local function get_mode()

    if using_random_mode then
        
        local chosen_gamemode = gamemodes[math.random(#gamemodes)]
        
        if chosen_gamemode == server.gamemode or chosen_gamemode == "coop-edit" then
            return get_mode()
        end
        
    end
    
    return server.gamemode
    
end

local function random_map(mode)
    
    local maplist = get_maplist(mode)
    local chosen_map = maplist[math.random(#maplist)]
    
    if chosen_map == server.map then
        return random_map(mode)
    end
    
    return chosen_map
end

local function get_nextgame(mode, map)
    
    if not mode then
        mode = get_mode()
    end
    
    if not map then
        
        if using_random_map then
            map = random_map(mode)
        else
            map = nextmap(mode, gamecount) or map
        end        
    end
    
    return mode, map
end

server.event_handler("setnextgame",function()
    
    if not using_server_maprotation then return end
    
    local mode
    if gamecount == 0 then mode = default_gamemode end
    
    server.next_mode, server.next_map = get_nextgame(mode)
    
    gamecount = gamecount + 1
end)

server.event_handler("disconnect", function()

    if server.playercount == 0 and default_game_on_empty and using_server_maprotation then
        
        local mode = default_gamemode
        local map = nil
        
        mode, map = get_nextgame(mode)
        
        server.changemap(map, mode, -1)
        gamecount = gamecount + 1
    end

end)

local function def_nextmap(gmode, cmap)

    local mlist = client_maps[gmode]
    
    if not mlist then
        return nil
    end
    
    local index = -1
    for i,name in ipairs(mlist) do
        if name == cmap then
            index = i
            break
        end
    end
    
    if index == -1 then
        return nil
    end
    
    return mlist[(index % #mlist) + 1]
end

local function nextmap_command(cn)

    if using_server_maprotation then
    
        if server.use_server_random_moderotation == 1 then
            server.player_msg(cn, "Note: The next mode will be chosen randomly at the end of this game, therefore the next map, too.")
        elseif server.use_server_random_maprotation == 1 then
            server.player_msg(cn, "The next map will be chosen randomly at the end of this game.")
        else
            local nm = nextmap(tostring(server.gamemode), gamecount)
            if nm then
                server.player_msg(cn, "The next map is " .. green(nm) .. ".")
                if bestmapsize == 1 and not gamemodeinfo.teams then
                    server.player_msg(cn, "Note: The next map will be determined on the number of players still connected at the end of this game.")
                end
            end
        end
        
    else
    
        local nm = def_nextmap(tostring(server.gamemode),tostring(server.map))
        if nm then
            server.player_msg(cn, "The next map is " .. green(nm) .. ".")
        end
    end
    
end

player_command_function("nextmap", nextmap_command)
