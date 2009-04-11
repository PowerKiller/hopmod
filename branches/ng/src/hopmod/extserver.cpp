#ifdef INCLUDE_EXTSERVER_CPP

void shutdown()
{
    signal_shutdown();
    exit(0);
}

struct kickinfo
{
    int cn;
    int sessionid;
    int time; //seconds
    std::string admin;
    std::string reason;
};

static int execute_kick(void * vinfoptr)
{
    kickinfo * info = (kickinfo *)vinfoptr;
    clientinfo * ci = (clientinfo *)getinfo(info->cn);
    
    if(!ci || ci->sessionid != info->sessionid)
    {
        delete info;
        return 0;
    }
    
    std::string full_reason;
    if(info->reason.length())
    {
        full_reason = (info->time == 0 ? "kicked for " : "kicked and banned for ");
        full_reason += info->reason;
    }
    ci->disconnect_reason = full_reason;
    
    allowedips.removeobj(getclientip(ci->clientnum));
    netmask addr(getclientip(ci->clientnum));
    if(info->time == -1) bannedips.set_permanent_ban(addr);
    else bantimes.set_ban(addr,info->time);
    
    signal_kick(info->cn, info->time, info->admin, info->reason);
    
    disconnect_client(info->cn, DISC_KICK);
    
    delete info;
    return 0;
}

void kick(int cn,int time,const std::string & admin,const std::string & reason)
{
    clientinfo * ci = get_ci(cn);
    
    kickinfo * info = new kickinfo;
    info->cn = cn;
    info->sessionid = ci->sessionid;
    info->time = time;
    info->admin = admin;
    info->reason = reason;
    
    sched_callback(&execute_kick, info);
}

void changetime(int remaining)
{
    gamelimit = gamemillis + remaining;
    if(!gamepaused) checkintermission();
}

void clearbans()
{
    bantimes.clear();
}

void player_msg(int cn,const char * text)
{
    get_ci(cn)->sendprivtext(text);
}

int player_id(int cn){return get_ci(cn)->playerid;}
int player_sessionid(int cn){return get_ci(cn)->sessionid;}
const char * player_name(int cn){return get_ci(cn)->name;}
const char * player_team(int cn){return get_ci(cn)->team;}
const char * player_ip(int cn){return get_ci(cn)->hostname();}
int player_iplong(int cn){return getclientip(get_ci(cn)->clientnum);}
int player_status_code(int cn){return get_ci(cn)->state.state;}
int player_ping(int cn){return get_ci(cn)->ping;}
int player_frags(int cn){return get_ci(cn)->state.frags;}
int player_deaths(int cn){return get_ci(cn)->state.deaths;}
int player_suicides(int cn){return get_ci(cn)->state.suicides;}
int player_teamkills(int cn){return get_ci(cn)->state.teamkills;}
int player_damage(int cn){return get_ci(cn)->state.damage;}
int player_maxhealth(int cn){return get_ci(cn)->state.maxhealth;}
int player_health(int cn){return get_ci(cn)->state.health;}
int player_gun(int cn){return get_ci(cn)->state.gunselect;}
int player_hits(int cn){return get_ci(cn)->state.hits;}
int player_shots(int cn){return get_ci(cn)->state.shots;}
int player_accuracy(int cn)
{
    clientinfo * ci = get_ci(cn);
    
    int hits = ci->state.hits;
    int shots = ci->state.shots;
    if(!shots) return -1;
    
    return static_cast<int>(roundf(static_cast<float>(hits)/shots*100));
}

int player_privilege_code(int cn){return get_ci(cn)->privilege;}

const char * player_privilege(int cn)
{
    switch(get_ci(cn)->privilege)
    {
        case PRIV_MASTER: return "master";
        case PRIV_ADMIN: return "admin";
        default: return "none";
    }
}

const char * player_status(int cn)
{
    switch(get_ci(cn)->state.state)
    {
        case CS_ALIVE: return "alive";
        case CS_DEAD: return "dead"; 
        case CS_SPAWNING: return "spawning"; 
        case CS_LAGGED: return "lagged"; 
        case CS_SPECTATOR: return "spectator";
        case CS_EDITING: return "editing"; 
        default: return "unknown";
    }
}

int player_connection_time(int cn)
{
    return (totalmillis - get_ci(cn)->connectmillis)/1000;
}

int player_timeplayed(int cn)
{
    clientinfo * ci = get_ci(cn);
    return (ci->state.timeplayed + (ci->state.state != CS_SPECTATOR ? (lastmillis - ci->state.lasttimeplayed) : 0))/1000;
}

void player_slay(int cn)
{
    clientinfo * ci = get_ci(cn);
    if(ci->state.state != CS_ALIVE) return;
    ci->state.state = CS_DEAD;
    sendf(-1, 1, "ri2", SV_FORCEDEATH, cn);
}

bool player_changeteam(int cn,const char * newteam)
{
    clientinfo * ci = get_ci(cn);
    if(!m_teammode || ci->state.state == CS_SPECTATOR || 
        (!smode || !smode->canchangeteam(ci, ci->team, newteam)) ||
        signal_chteamrequest(cn, ci->team, newteam) == -1) return false;
    
    if(smode) smode->changeteam(ci, ci->team, newteam);
    signal_reteam(ci->clientnum, ci->team, newteam);
    
    copystring(ci->team, newteam, MAXTEAMLEN+1);
    sendf(-1, 1, "riis", SV_SETTEAM, cn, newteam);
    
    if(ci->state.aitype == AI_NONE) aiman::dorefresh = true;
    
    return true;
}

void changemap(const char * map,const char * mode = "",int mins = -1)
{
    int gmode = (mode[0] ? modecode(mode) : gamemode);
    if(!m_mp(gmode)) gmode = gamemode;
    sendf(-1, 1, "risii", SV_MAPCHANGE, map, gmode, 1);
    changemap(map,gmode,mins);
}

int getplayercount()
{
    return numclients(-1, false, true);
}

int getbotcount()
{
    return numclients(-1, true, false) - numclients();
}

int getspeccount()
{
    return getplayercount() - numclients();
}

void team_msg(const char * team,const char * msg)
{
    if(!m_teammode) return;
    defformatstring(line)("server: " BLUE "%s",msg);
    loopv(clients)
    {
        clientinfo *t = clients[i];
        if(t->state.state==CS_SPECTATOR || t->state.aitype != AI_NONE || strcmp(t->team, team)) continue;
        t->sendprivtext(line);
    }
}

void player_spec(int cn)
{
    setspectator(get_ci(cn), true);
}

void player_unspec(int cn)
{
    setspectator(get_ci(cn), false);
}

int player_bots(int cn)
{
    clientinfo * ci = get_ci(cn);
    return ci->bots.length();
}

void unsetmaster()
{
    if(currentmaster != -1)
    {
        clientinfo * master = getinfo(currentmaster);
        
        defformatstring(msg)("The server has revoked your %s privilege.",privname(master->privilege));
        master->sendprivtext(msg);
        
        master->privilege = 0;
        mastermode = MM_OPEN;
        allowedips.setsize(0);
        currentmaster = -1;
        masterupdate = true;
    }
}

void setpriv(int cn,int priv,bool hidden)
{
    clientinfo * player = get_ci(cn);
    if(player->privilege == priv) return;
    unsetmaster();
    player->privilege = priv;
    currentmaster = cn;
    masterupdate = true;
    
    defformatstring(msg)("The server has raised your privilege to %s.", privname(priv));
    player->sendprivtext(msg);
}

void server_setmaster(int cn)
{
    setpriv(cn, PRIV_MASTER, false);
}

void server_setadmin(int cn)
{
    setpriv(cn, PRIV_ADMIN, false);
}

bool writebanlist(const char * filename)
{
    return write_banlist(&bannedips, filename);
}

bool loadbanlist(const char * filename)
{
    return load_banlist(filename, &bannedips);
}

void addpermban(const char * addrstr)
{
    netmask addr;
    try{addr = netmask::make(addrstr);}
    catch(std::bad_cast){throw fungu::script::error(fungu::script::BAD_CAST);}
    bannedips.set_permanent_ban(addr);
}

void unsetban(const char * addrstr)
{
    netmask addr;
    try{addr = netmask::make(addrstr);}
    catch(std::bad_cast){throw fungu::script::error(fungu::script::BAD_CAST);}
    bannedips.unset_ban(addr);
}

void delbot(int cn)
{
    clientinfo * ci = get_ci(cn);
    aiman::reqdel(ci);
}

void enable_master_auth(bool enable)
{
    mastermask = (enable ? mastermask & ~MM_AUTOAPPROVE : mastermask | MM_AUTOAPPROVE);
}

void update_mastermask()
{
    bool autoapprove = mastermask & MM_AUTOAPPROVE;
    mastermask &= ~(1<<MM_VETO) & ~(1<<MM_LOCKED) & ~(1<<MM_PRIVATE) & ~MM_AUTOAPPROVE;
    mastermask |= (allow_mm_veto << MM_VETO);
    mastermask |= (allow_mm_locked << MM_LOCKED);
    mastermask |= (allow_mm_private << MM_PRIVATE);
    if(autoapprove) mastermask |= MM_AUTOAPPROVE;
}

const char * gamemodename()
{
    return modename(gamemode,"unknown");
}

std::vector<int> cs_clients_list(bool (* clienttype)(clientinfo *))
{
    std::vector<int> result;
    result.reserve(clients.length());
    loopv(clients) if(clienttype(clients[i])) result.push_back(clients[i]->clientnum);
    return result;
}

int lua_clients_list(lua_State * L,bool (* clienttype)(clientinfo *))
{
    lua_newtable(L);
    int count = 0;
    
    loopv(clients) if(clienttype(clients[i]))
    {
        lua_pushinteger(L,++count);
        lua_pushinteger(L,clients[i]->clientnum);
        lua_settable(L, -3);
    }
    
    return 1;
}

bool is_player(clientinfo * ci){return ci->state.state != CS_SPECTATOR && ci->state.aitype == AI_NONE;}
bool is_spectator(clientinfo * ci){return ci->state.state == CS_SPECTATOR; /* bots can't be spectators*/}

std::vector<int> cs_player_list(){return cs_clients_list(&is_player);}
int lua_player_list(lua_State * L){return lua_clients_list(L, &is_player);}
std::vector<int> cs_spec_list(){return cs_clients_list(&is_spectator);}
int lua_spec_list(lua_State * L){return lua_clients_list(L, &is_spectator);}

int recorddemo()
{
    if(demorecord) return demo_id;
    else return setupdemorecord(false);
}

int lua_gamemodeinfo(lua_State * L)
{
    lua_newtable(L);
    
    #define INFO_FIELD(m, name) \
        lua_pushboolean(L, m); \
        lua_setfield(L, -2, name) 
    INFO_FIELD(m_noitems, "noitems");
    INFO_FIELD(m_noammo, "noammo");
    INFO_FIELD(m_insta, "insta");
    INFO_FIELD(m_tactics, "tactics");
    INFO_FIELD(m_efficiency, "efficiency");
    INFO_FIELD(m_capture, "capture");
    INFO_FIELD(m_regencapture, "regencapture");
    INFO_FIELD(m_ctf, "ctf");
    INFO_FIELD(m_protect, "protect");
    INFO_FIELD(m_teammode, "teams");
    INFO_FIELD(m_overtime, "overtime");
    INFO_FIELD(m_timed, "timed");
    #undef INFO_FIELD
    
    return 1;
}

#endif
