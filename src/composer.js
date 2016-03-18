var engine;
var scene;
var seqs = [150];
var start_node = null;
var start_bullet = null;
var seq_bullets = [];
var sliding = false;
var paused = false;
var seqindex = 0;
var frame_in_seq = 0;
var mirror_view = false;
var keyframes;
var thepos;
var externalCamera;
var firstPersonCamera;

function seq_index_to_frame_index(s)
{
	var r = 0;
	for (var i = 0; i != s; ++i)
		r += transitions[seqs[i]].frames.length - 1;
	return r;
}

function frame_index_to_seq(f)
{
	if (f < 0) f = 0;

	var r = 0;

	for (var s = 0; s != seqs.length; ++s)
	{
		var l = transitions[seqs[s]].frames.length;
		if (f < l) return [s, f];
		f -= l - 1;
	}

	return [seqs.length - 1, 0]; // todo: not 0
}

function on_pause_button_clicked()
{
	paused = !paused;
	document.getElementById("pause_button").innerHTML = (paused ? "play" : "pause");
}

function on_pop_front_button_clicked()
{
	engine.stopRenderLoop();
	start_node = transitions[seqs[0]].to;

	if (seqindex == 0)
	{
		frame = 0;
		frame_in_seq = 0;
	}
	else
	{
		--seqindex;
		frame = seq_index_to_frame_index(seqindex) + frame_in_seq;
	}

	seqs.splice(0, 1);
	resetScene();
}

function on_pop_back_button_clicked()
{
	engine.stopRenderLoop();

	if (seqs.length == 1)
	{
		seqs = [];
		seqindex = 0;
		frame_in_seq = 0;
		frame = 0;
	}
	else
	{
		if (seqindex == seqs.length - 1)
		{
			--seqindex;
			frame_in_seq = 0;
			frame = seq_index_to_frame_index(seqindex) + frame_in_seq;
		}

		seqs.splice(seqs.length - 1, 1);
	}

	resetScene();
}

function refreshPreChoices()
{
	var elem = document.getElementById("pre_choices");

	elem.innerHTML = "";

	var choices = nodes[start_node].incoming;

	for (var i = 0; i != choices.length; ++i)
	{
		var choice = choices[i];
		var from_node = transitions[choice].from;

		var btn = document.createElement("button");
		btn.style.textAlign = "left";
		btn.appendChild(document.createTextNode(nodes[from_node].description));
		btn.appendChild(document.createElement("br"));
		btn.appendChild(document.createTextNode(indent + "↳ " + transitions[choice].description));
		btn.addEventListener("click", function(c){ return function(){
				seqs = [c].concat(seqs);
				start_node = transitions[c].from;
				resetScene();
			}}(choice));

		elem.appendChild(btn);
		elem.appendChild(document.createElement("br"));
	}
}

var indent = "\u00a0 \u00a0 \u00a0 ";

function refreshPostChoices()
{
	var elem = document.getElementById("post_choices");

	elem.innerHTML = "";

	var end_node = (seqs.length == 0)
		? start_node
		: transitions[seqs[seqs.length - 1]].to;

	var choices = nodes[end_node].outgoing;

	document.getElementById("post_choices_label").style.display = choices.length == 0 ? 'none' : 'inline';

	for (var i = 0; i != choices.length; ++i)
	{
		var choice = choices[i];
		var to_node = transitions[choice].to;

		var btn = document.createElement("button");
		btn.style.textAlign = "left";
		btn.appendChild(document.createTextNode(transitions[choice].description));
		btn.appendChild(document.createElement("br"));
		btn.appendChild(document.createTextNode(indent + "↳ " + nodes[to_node].description));

		btn.addEventListener("click", function(c){ return function(){
				seqs.push(c);
				resetScene();
			}}(choice));

		elem.appendChild(btn);
		elem.appendChild(document.createElement("br"));
	}
}

function node_link(node)
{
	var link = document.createElement("a");
	link.href = "../p" + node + "n.html";
	link.text = nodes[node].description;
	return link;
}

function pick_bullet()
{
	for (var i = 0; i != seqs.length; ++i)
		seq_bullets[i].style.visibility
			= (i == seqindex ? 'visible' : 'hidden');
}

function make_bullet()
{
	var b = document.createElement("div");
	b.appendChild(document.createTextNode("▸ \u00a0 "));
	b.style.display = 'inline';
	b.style.visibility = 'hidden';
	return b;
}

function refreshDrill()
{
	var controls = document.getElementById("controls"); // todo: rename
	controls.innerHTML = "";

	controls.appendChild(start_bullet = make_bullet());

	var starttag = document.createElement("i");
	starttag.innerHTML = "Start: \u00a0";

	controls.appendChild(starttag);
	controls.appendChild(node_link(start_node));

	if (seqs.length != 0)
		controls.appendChild(document.createElement("hr"));

	seq_bullets = [];

	for (var i = 0; i != seqs.length; ++i)
	{
		var bullet = make_bullet();
		seq_bullets.push(bullet);
		controls.appendChild(bullet);

		var seq = seqs[i];
		var seqLabel = document.createElement("a");
		seqLabel.text = (i+1) + ". \u00a0" + transitions[seq].description + " \u00a0";

		controls.appendChild(seqLabel);

		if (i == 0 && seqs.length >= 2)
		{
			var btn = document.createElement("button");
			btn.appendChild(document.createTextNode("x"));
			btn.addEventListener("click", on_pop_front_button_clicked);
			controls.appendChild(btn);
		}
		else if (i == seqs.length - 1/* && seqs.length >= 1*/)
		{
			var btn = document.createElement("button");
			btn.appendChild(document.createTextNode("x"));
			btn.addEventListener("click", on_pop_back_button_clicked);
			controls.appendChild(btn);
		}

		var to_node = transitions[seq].to;

		controls.appendChild(document.createElement("br"));
		controls.appendChild(document.createTextNode("\u00a0 \u00a0 \u00a0 \u00a0 \u00a0 \u00a0 \u00a0 ↳ \u00a0")); // todo
		controls.appendChild(node_link(to_node));
		controls.appendChild(document.createElement("br"));
	}

	if (seqs.length != 0)
	{
		var sharelink = document.createElement("a");
		sharelink.href="?" + seqs.join(',');
		sharelink.text = "share";
		controls.appendChild(document.createTextNode("("));
		controls.appendChild(sharelink);
		controls.appendChild(document.createTextNode(")"));
	}
}

function on_slide()
{
	frame = parseInt(document.getElementById("slider").value);
	var x = frame_index_to_seq(frame);
	seqindex = x[0];
	frame_in_seq = x[1];
	k = 0;
	pick_bullet();
}

function interpolate(a, b, c)
{
	return a.scale(1 - c).add(b.scale(c));
}

function keyframe(i)
{
	return keyframes[bound_frame_index(i, keyframes.length)];
}

function interpolate_position(a, b, c)
{
	var p = [[],[]];

	for (var pl = 0; pl != 2; ++pl)
	for (var j = 0; j != joints.length; ++j)
		p[pl].push(interpolate(a[pl][j], b[pl][j], c));

	return p;
}

function on_view_change()
{
	var vv = document.getElementById("view_select").value;

	scene.activeCamera = (vv == "external" ? externalCamera : firstPersonCamera);
}

function updateCamera()
{
	var vv = document.getElementById("view_select").value;
	if (vv == "external") return;
	var pl = parseInt(vv);
	var opp = 1 - pl;

	firstPersonCamera.setTarget(
		thepos[pl][LeftFingers]
		.add(thepos[pl][RightFingers])
		.add(thepos[opp][Head].scale(2))
		.scale(0.25));

	firstPersonCamera.position = thepos[pl][Head];
}

function updateDrawnPos()
{
	var therealpos = interpolate_position(
		keyframe(frame),
		keyframe(frame + 1), k);

	thepos = (thepos ? interpolate_position(thepos, therealpos, drag) : therealpos);
}

function tick()
{
	if (!sliding && !paused)
	{
		k += engine.getDeltaTime() / speed;
		if (k >= 1)
		{
			k -= 1;
			if (k <= 0) k = 0;

			++frame_in_seq;

			var last_frame_in_seq = transitions[seqs[seqindex]].frames.length - 1;
			if (seqindex == seqs.length - 1) last_frame_in_seq += 6;

			if (frame_in_seq == last_frame_in_seq)
			{
				++seqindex;
				if (seqindex == seqs.length)
				{
					seqindex = 0;
					frame_in_seq = -5;
				}
				else frame_in_seq = 0;

				pick_bullet();
			}

			frame = seq_index_to_frame_index(seqindex) + frame_in_seq;
			document.getElementById("slider").value = frame;
		}
	}

	updateDrawnPos();


	updateCamera();
}

function follow(seqs, mirror)
{
	var reo;
	var r = [];

	var cores = v3(0,0,0);

	for (var i = 0; i != seqs.length; ++i)
	{
		var seq = seqs[i];
		var trans = transitions[seq];
		var newframes = trans.frames;

		if (r.length != 0) r.pop();

		if (i != 0)
			reo = compose_reo(
				compose_reo(
					inverse_reo(transitions[seqs[i]].reo_from),
					transitions[seqs[i-1]].reo_to),
				reo);
		else
		{
			reo = transitions[seqs[i]].reo_from;
			if (mirror) reo.mirror = !reo.mirror;
		}

		for (var j = 0; j != newframes.length; ++j)
		{
			var f = apply_reo(reo, newframes[j]);
			r.push(f);
			cores = cores.add(f[0][Core]).add(f[1][Core]);
		}
	}

	cores.y = 0;

	var centerer = { mirror: false, swap_players: false, angle: 0, offset: cores.scale(-1 / (r.length * 2)) };
	for (var i = 0; i != r.length; ++i)
		r[i] = apply_reo(centerer, r[i]);

	return r;
}

function on_mirror_button_clicked()
{
	for (var f = 0; f != keyframes.length; ++f)
		mirror(keyframes[f]);
}

function resetScene()
{
	engine.stopRenderLoop();

	if (scene) scene.dispose();

	if (seqs.length == 0)
	{
		keyframes = [nodes[start_node].position];
		if (mirror_view) mirror(keyframes[0]);
	}
	else keyframes = follow(seqs, mirror_view);

	scene = showpos(keyframes[0], engine);

	document.getElementById("slider").max = keyframes.length - 1;

	frame = -1;
	k = 0;
	frame_in_seq = -1;

	refreshDrill();
	refreshPreChoices();
	refreshPostChoices();

	if (seqs.length != 0) scene.registerBeforeRender(tick);

	on_view_change();

	engine.runRenderLoop(function(){ scene.render(); });
}

window.addEventListener('DOMContentLoaded',
	function()
	{
		var s = window.location.href;
		var qmark = s.lastIndexOf('?');
		if (qmark != -1)
		{
			arg = s.substr(qmark + 1);

			if (arg[0] == 'p')
			{
				start_node = arg.substr(1);
				seqs = [];
			}
			else
			{
				seqs = arg.split(',');
				start_node = transitions[seqs[0]].from;
			}
		}

		var canvas = document.getElementById('renderCanvas');
		engine = new BABYLON.Engine(canvas, true);
		resetScene();
	});

function showpos(pos, engine)
{
	var canvas = document.getElementById('renderCanvas');

	var scene = new BABYLON.Scene(engine);

	scene.clearColor = new BABYLON.Color3(1,1,1);

	externalCamera = new BABYLON.ArcRotateCamera("ArcRotateCamera",
		0, // rotation around Y axis
		3.14/4, // rotation around X axis
		2.5, // radius
		new BABYLON.Vector3(0, 0, 0),
		scene);

	externalCamera.wheelPrecision = 30;
	externalCamera.lowerBetaLimit = 0;
	externalCamera.upperRadiusLimit = 4;
	externalCamera.lowerRadiusLimit = 1;

	firstPersonCamera = new BABYLON.FreeCamera("FreeCamera", new BABYLON.Vector3(0, 1, -5), scene);
	firstPersonCamera.minZ = 0.05;

	externalCamera.attachControl(canvas);

	var light = new BABYLON.HemisphericLight('light1', new BABYLON.Vector3(0,1,0), scene);

	var draw = animated_position_from_array(pos, scene);

	scene.registerBeforeRender(function(){ if (thepos) draw(thepos); });

	return scene;
}
