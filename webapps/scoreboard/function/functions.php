<?php
/*
 * (c) 2011 by Thomas Poechtrager
 */
if (!$is_included) exit();

function escape_sql($val, $is_num=false) {
    if (!$is_num) {
        if ($config['use_mysql'] && function_exists("mysql_real_escape_string")) {
            $val = mysql_real_escape_string($val);
        }
        else {
            if (!$config['use_mysql'] && function_exists("sqlite_escape_string")) {
                $val = sqlite_escape_string($val);
            }
            else {
                $val = addslashes($val);
            }
        }
    }
    else {
        for($i = 0; $i <= strlen($val)/*bad way but works*/; $i++)
            $val[$i] = is_numeric($val[$i]) ? $val[$i] : '';
    }
    return $val;
}

function format_time($time) {
    $weeks = floor($time / (86400 * 7));
    $days = floor(($time - ($weeks * 86400 * 7)) / 86400);
    $hrs = floor(($time - (($weeks * 86400 * 7) + ($days * 86400))) / 3600);
    $min = floor(($time - (($weeks * 86400 * 7) + ($days * 86400) + ($hrs * 3600))) / 60);
    $sec = $time - (($weeks * 86400 * 7) + ($days * 86400) + ($hrs * 3600) + ($min * 60));
    
    $ftime = "";
    $ftime .= $weeks > 0 ? $weeks."w " : "";
    $ftime .= $days > 0 ? $days."d " : "";
    $ftime .= $hrs > 0 ? $hrs."h " : "";
    $ftime .= $min > 0 ? $min."m " : "";
    $ftime .= $sec > 0 ? $sec."s " : "";
    
    return $ftime;
}

function convert_timestamp($timestamp) {
    global $config;
    return $config['use_mysql'] ? $timestamp : date("d-m-Y H:i:s", $timestamp);
}

function color($color, $val) {
    return '<font color="'.$color.'">'.$val.'</font>';
}

function get_args($block = array()) {
	$args = "";
	$arg_keys = array_keys($_GET);
	for($i = 0; $i < count($arg_keys); $i++) {
		$key = $arg_keys[$i];
		if (in_array($key, $block)) continue;
		$arg = $_GET[$key];
		$args .= "&";
		$args .= htmlspecialchars($key.'='.$arg);
	}
	return $args;
}

function exclude_names() {
    global $config;
    $syntax = "";
    $i = 0;
    if (sizeof($config['exclude_names']) > 0) {
        foreach($config['exclude_names'] as $name => $ip) { 
            if ($i++ > 0) $syntax .= " OR ";
            if ($ip != "") $syntax .= "(";
            $syntax .= "name = '".escape_sql($name)."'";
            if ($ip != "") $syntax .= " AND ipaddr LIKE '%".escape_sql($ip)."%')";
        }
    }
    return $syntax == "" ? "1=1" : "NOT ({$syntax})"; //FIXME
}

function get_msec() {
	list($usec, $sec) = explode(" ", microtime());
	$milliseconds = (((float)$usec/1000) + (float)$sec); 
	return $milliseconds;
}

function start_benchmark() { global $start; $start = get_msec(); } 	
function end_benchmark() { 
	global $start; 
	$diff = get_msec() - $start;
	echo "Took ".$diff. " Seconds to generate this page";
} 	


function td($val, $spec=true) {
	return "<td>".($spec ? htmlspecialchars($val) : $val)."</td>";
}

function tr2($val1, $val2, $spec=true) {
    return '<tr><td>'.($spec ? htmlspecialchars($val1) : $val1).'</td><td>'.($spec ? htmlspecialchars($val2) : $val2).'</td></tr>';
}


function get_player_table($page=1, $days=0, $sel_player="") {
	global $config;
	
	if ($page == NULL || $page == "" || $page == 0 || !is_numeric($page)) $page = 1;
	
	$sql = "";
    
    if ($sel_player != "") { 
        $sel_player = escape_sql($sel_player);
    }
	
	if ($days > 0) {
		$sql = "
			SELECT name, sum(frags) AS frags, sum(deaths) AS deaths, sum(teamkills) AS teamkills, sum(suicides) AS suicides, sum(win) AS wins, sum(timeplayed) AS timeplayed, count(*) AS games
			FROM players, games
			WHERE games.id = players.game_id AND games.datetime > ".(time() - $days*60*60*24).($sel_player!=""? " AND name LIKE '%".$sel_player."%'" : "")." AND ".exclude_names()."
			GROUP BY name
			ORDER BY sum(frags) DESC
			LIMIT {$config['res_per_page']} OFFSET ".($config['res_per_page']*($page - 1)); 
	}
	else {
        if ($sel_player != "") $sel_player = " AND name LIKE '%{$sel_player}%'";
		$sql = "SELECT * FROM playertotals WHERE ".exclude_names()." {$sel_player} ORDER BY frags DESC LIMIT {$config['res_per_page']} OFFSET ".($config['res_per_page']*($page - 1)); 
	}
	
	$totals = read_array($sql);
	
	if (!$totals) return false;
	
	$table = "";
	
	$table .= '<table border="1" id="scoreboard" class="tablesorter" style="width:'.$config['table_width'].'px;">';
	$table .= '<thead>';
	$table .= '<th>Rank</th><th>Name</th>'.($config['show_country']?'<th>Country</th>':'').'<th>Frags</th><th>Deaths</th><th>KpD</th>'.($config['show_kpg']?'<th>KpG</th>':'').'<th>Suicides</th><th>Teamkills</th><th>Accuracy</th><th>Games Played</th><th>Time Played</th><th>Wins</th>';
	$table .= '</thead>';
	
	for ($i = 0; $i < count($totals) - 1; $i++) {
		
        $player = $totals[$i];
		
        $table .= "<tr>";
        $table .= td(($page - 1) * $config['res_per_page'] + $i + 1);
        $table .= td($player["name"]);
        if ($config['show_country']) $table .= td(get_country_name($player["ipaddr"]));
        $table .= td($player["frags"]);
        $table .= td($player["deaths"]);
        $table .= td(round($player["frags"] / ($player["deaths"] ? $player["deaths"] : 1), 2));
        if ($config['show_kpg']) $table .= td(round($player["frags"] / ($player["games"] ? $player["games"] : 1), 2));
        $table .= td($player["suicides"]);
        $table .= td($player["teamkills"]);
        $table .= td(round($player["damage"] / (($player["damage"]+$player["damagewasted"]) ? ($player["damage"]+$player["damagewasted"]) : 1) * 100, 2)."%");
        $table .= td($player["games"]);
        $table .= td(format_time($player["timeplayed"]));
        $table .= td($player["wins"]);
        $table .= "</tr>";
	}
	
	$table .= '</table>';
	
    unset($totals);
	
    if ($days > 0) {
        $sql_count = "
            SELECT count(players.name) FROM players, games
            WHERE players.game_id = games.id AND games.datetime > ".(time() - $days*60*60*24).($player!=""? " AND name LIKE '%".$player."%'" : "")."
            GROUP by players.name
        ";
    }
    else {
        $sql_count = "SELECT count(*) AS count FROM playertotals {$sel_player}";
    }
	
    $count = $days > 0 ? array("count" => count(read_array($sql_count))) : read($sql_count);

    return array($table, $count["count"]);
}

function get_gamelist_table($page=1, $days=0) {
	global $config;
	
	if ($page == NULL || $page == "" || $page == 0 || !is_numeric($page)) $page = 1;
	
	$sel_games = $days > 0 ? "WHERE datetime > ".(time() - $days*60*60*24) : "";
	$games = read_array("SELECT * FROM games {$sel_games} ORDER BY datetime DESC LIMIT {$config['res_per_page']} OFFSET ".($config['res_per_page']*($page - 1)));
	
	if (!$games) return false;
	
	$table = "";
	
	$table .= '<table border="1" id="scoreboard" class="tablesorter" style="width:800px;">';
	$table .= '<thead>';
	$table .= '<th>Time</th><th>Mode</th><th>Map</th><th>Duration</th><th>Players</th><th>Bots</th>';
	$table .= '</thead>';	

	
	for ($i = 0; $i < count($games) - 1; $i++) {
		
		$game = $games[$i];
		
		$table .= "<tr>";
		$table .= td('<a href="?game_id='.$game["id"].'">'.(convert_timestamp($game["gametime"])).'</a>', false);
		$table .= td($game["gamemode"]);
		$table .= td($game["mapname"]);
		$table .= td($game["duration"]." Minutes");
		$table .= td($game["players"]);
		$table .= td($game["bots"]);
		$table .= "</tr>";
	}

	$table .= '</table>';
	
	$count = read("SELECT count(*) AS count FROM games {$sel_games} ORDER BY datetime DESC");	
	
	return array($table, $count["count"]);
}

function get_game_table($game_id) {
	global $config;
		
	if ($game_id == NULL || $game_id == "" || $game_id == 0 || !is_numeric($game_id)) 
		return false;
	
	$game = read("SELECT * FROM games WHERE id = {$game_id}");
	$players = read_array("SELECT * FROM players WHERE game_id = {$game_id}");
	$teams = read_array("SELECT * FROM teams WHERE game_id = {$game_id} ORDER BY score DESC");
	
	$is_teammode = strstr($game["gamemode"], "ctf") || strstr($game["gamemode"], "team");
    
	$t = "";
    
    $t .= '<font face="Arial" size="+2">'.
        color('red', $game["gamemode"]).' '.
        color('blue', $game["mapname"]).' '.
        color('red', $game["duration"].' Minutes').' '.
        color('blue', $game["players"].' Players').'</font>';
	
	$team_tables = array();
    
	
	for ($i = 0; $i < count($players); $i++) {
		$player = $players[$i];
		
		$team = array();
		
		for ($j = 0; $j < count($teams); $j++) {
			$team = $teams[$j];
			if ($team["id"] == $player["team_id"])
				break;
		}
		
		$table = &$team_tables[($is_teammode ? $team["name"] : 0)];
		
		$table .= "<tr>";
		$table .= td($player["name"]);
		$table .= td($player["frags"]);
		$table .= td($player["deaths"]);
		$table .= td($player["suicides"]);
		$table .= td($player["teamkills"]);
		$table .= td($player["timeplayed"]);
		$table .= "</tr>";
		
	}

	if ($is_teammode) $t .= '<table><tr>';
	
	$team_head = "";

	$team_head .= '<thead>';
	$team_head .= '<th>Name</th><th>Frags</th><th>Deaths</th><th>Suicides</th><th>Teamkills</th><th>Timeplayed</th></th>';
	$team_head .= '</thead>';	
	
	for ($i = 0; $i < count($teams)-1; $i++) {
	
		if ($is_teammode) $t .= '<td valign="top">';
		
		$team = $team_tables[$teams[$i]['name']];
        $team_ = $teams[$i];
		
		$t .= '<table border="1" id="scoreboard" class="tablesorter" style="width:500px;">';
        
        $win = "";
      
        if ($team_["draw"]) $win = "*DRAW*"; else $win = ($team_["win"] ? "*WINNER*" : "");
        
        $team_info = '
            <thead>
                <td colspan="6">
                    <font size="+1">'.htmlspecialchars($team_['name']).' - score: '.$team_["score"].' '.$win.'</font>
                </td>
            </thead>';
        
        
        $t .= $team_info;
		$t .= $team_head;
		$t .= $team;
		$t .= "</table>";
		
		if ($is_teammode) $t .= "</td>";
		
	}
	
	if ($is_teammode) $t .= "</tr></table>";
	
	if ($_SERVER["HTTP_REFERER"] != "")
		$t .= '<br><br><a href="'.htmlspecialchars($_SERVER["HTTP_REFERER"]).'">go back</a>';
	
	return array($t, 0);
}

function generate_pagelist($count, $page) {
	global $config;
	if ($count == 0) return "";
	if ($page == NULL || $page == "" || $page == 0 || !is_numeric($page)) $page = 1;
    
	$pages = $count / $config['res_per_page'];
    if ($count % $config['res_per_page'] != 0) $pages++;//i know this way sucks..
    $pages = floor($pages);
    
    if ($page > $pages) {
        echo '<script>window.location = "?page='.$pages.get_args(array("page")).'"</script>'; // stupid but works :P
        
        exit();
    }
    
	$html = "";
	$page_before = max($page - 1, 1);
	
	$pages_to_display = 15;
		
	$start = $page - ($pages_to_display / 2);
	$end = $start + $pages_to_display;
	
	if ($end > $pages) {
		$start -= $end - $pages;
		$end = $start + $pages_to_display;
	}	
	
	if ($start <= 0) {
		$start = 1;
		$end = $start + $pages_to_display;
	}
    
    if ($end > $pages) $end = $pages;
	
	$args = get_args(array("page"));
		
	if ($start > 1) 
		$html .= "<a href='?page=1{$args}'><b>first</b></a>&nbsp;";
	
	$html .= "<a href='?page={$page_before}{$args}'><b>back</b></a>&nbsp;";
	
	for ($i = floor($start); $i <= $end; $i++) {
		$html .= "<a href='?page={$i}{$args}'>".($i == $page ? "<b>$i</b>" : $i)."</a>&nbsp;";
	}
	$next_page = min($page + 1, $pages);
	
	$html .= "<a href='?page={$next_page}{$args}'><b>next</b></a>&nbsp;";
	
	if ($end < $pages)
		$html .= "<a href='?page={$pages}{$args}'><b>last</b></a>";
		
	return $html;
}

?>