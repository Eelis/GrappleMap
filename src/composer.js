var engine;
var scene;
var steps = [{transition:217,reverse:false}];
var start_node = null;
var start_bullet = null;
var seq_bullets = [];
var sliding = false;
var paused = false;
var seqindex = 0;
var frame_in_seq = -5;
var mirror_view = false;
var keyframes;
var thepos;
var canvas;
var externalCamera;
var firstPersonCamera;

function seq_index_to_frame_index(s)
{
	var r = 0;
	for (var i = 0; i != s; ++i)
		r += transitions[steps[i].transition].frames.length - 1;
	return r;
}

function frame_index_to_seq(f)
{
	if (f < 0) f = 0;

	var r = 0;

	for (var s = 0; s != steps.length; ++s)
	{
		var l = transitions[steps[s].transition].frames.length;
		if (f < l) return [s, f];
		f -= l - 1;
	}

	return [steps.length - 1, 0]; // todo: not 0
}

function on_pause_button_clicked()
{
	paused = !paused;
	document.getElementById("pause_button").innerHTML = (paused ? "play" : "pause");
}

function on_pop_front_button_clicked()
{
	start_node = step_to(steps[0]).node;

	if (seqindex == 0)
	{
		frame = 0;
		frame_in_seq = 0;
		k = 0;
	}
	else
	{
		--seqindex;
		frame = seq_index_to_frame_index(seqindex) + frame_in_seq;
	}

	steps.splice(0, 1);

	resetFrames();
		// todo: preserve orientation so that animation
		//  can be left entirely undisturbed

	refreshDrill();
	refreshPreChoices();
}

function on_pop_back_button_clicked()
{
	if (steps.length == 1)
	{
		steps = [];
		seqindex = 0;
		frame_in_seq = 0;
		frame = 0;

		resetFrames();
		frame = -1;
		k = 0;
		frame_in_seq = -1;
		thepos = ideal_pos();
	}
	else
	{
		if (seqindex == steps.length - 1)
		{
			--seqindex;
			frame_in_seq = 0;
			frame = seq_index_to_frame_index(seqindex) + frame_in_seq;
			k = 0;
		}

		steps.splice(steps.length - 1, 1);

		resetFrames();
	}

	refreshDrill();
	refreshPostChoices();
}

function refreshPreChoices()
{
	var elem = document.getElementById("pre_choices");

	elem.innerHTML = "";

	var choices = nodes[start_node].incoming;

	for (var i = 0; i != choices.length; ++i)
	{
		var step = choices[i];
		var from_node = step_from(step).node;

		var btn = document.createElement("button");
		btn.style.margin = "3px";
		btn.style.textAlign = "left";
		btn.appendChild(document.createTextNode(nodes[from_node].description));
		btn.appendChild(document.createElement("br"));
		btn.appendChild(document.createTextNode(indent + "↳ " + transitions[step.transition].description[0]));
		btn.addEventListener("click", function(c){ return function(){
				steps = [c].concat(steps);
				start_node = step_from(c).node;
				resetFrames();
				frame = -1;
				k = 0;
				frame_in_seq = -1;
				thepos = ideal_pos();
				refreshDrill();
				refreshPreChoices();
			}}(step));

		elem.appendChild(btn);
	}
}

var indent = "\u00a0 \u00a0 \u00a0 ";

function refreshPostChoices()
{
	var elem = document.getElementById("post_choices");

	elem.innerHTML = "";

	var end_node = (steps.length == 0)
		? start_node
		: step_to(steps[steps.length - 1]).node;

	var choices = nodes[end_node].outgoing;

	document.getElementById("post_choices_label").style.display = choices.length == 0 ? 'none' : 'inline';

	for (var i = 0; i != choices.length; ++i)
	{
		var step = choices[i];
		var to_node = step_to(step).node;

		var btn = document.createElement("button");
		btn.style.margin = "3px";
		btn.style.textAlign = "left";
		btn.appendChild(document.createTextNode(transitions[step.transition].description[0]));
		btn.appendChild(document.createElement("br"));
		btn.appendChild(document.createTextNode(indent + "↳ " + nodes[to_node].description));

		btn.addEventListener("click", function(c){ return function(){
				steps.push(c);
				resetFrames();
				refreshDrill();
				refreshPostChoices();
			}}(step));

		elem.appendChild(btn);
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
	for (var i = 0; i != steps.length; ++i)
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

function encode_steps(a)
{
	var r = '';
	for (var i = 0; i != a.length; ++i)
	{
		if (r != '') r += ',';
		if (a[i].reverse) r += '-';
		r += a[i].transition;
	}
	return r;
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

	if (steps.length != 0)
		controls.appendChild(document.createElement("hr"));

	seq_bullets = [];

	for (var i = 0; i != steps.length; ++i)
	{
		var bullet = make_bullet();
		seq_bullets.push(bullet);
		controls.appendChild(bullet);

		var seq = steps[i].transition;
		var desc = transitions[seq].description;
		var seqLabel = document.createElement("a");
		seqLabel.text = (i+1) + ". \u00a0" + desc[0] + " \u00a0";

		controls.appendChild(seqLabel);

		// buttons:

		if (i == 0 && steps.length >= 2)
		{
			var btn = document.createElement("button");
			btn.appendChild(document.createTextNode("x"));
			btn.addEventListener("click", on_pop_front_button_clicked);
			controls.appendChild(btn);
		}
		else if (i == steps.length - 1/* && seqs.length >= 1*/)
		{
			var btn = document.createElement("button");
			btn.appendChild(document.createTextNode("x"));
			btn.addEventListener("click", on_pop_back_button_clicked);
			controls.appendChild(btn);
		}

		// link to node:

		controls.appendChild(document.createElement("br"));
		controls.appendChild(document.createTextNode("\u00a0 \u00a0 \u00a0 \u00a0 \u00a0 \u00a0 \u00a0 ↳ \u00a0")); // todo
		controls.appendChild(node_link(step_to(steps[i]).node));
		controls.appendChild(document.createElement("br"));

		// description:

		var descdiv = document.createElement("div");
		descdiv.style.marginLeft = "100px";
		descdiv.style.marginTop = "0px";

		for (var j = 1; j <= desc.length - 1; ++j)
		{
			if (j != 1) descdiv.appendChild(document.createElement("br"));
			descdiv.appendChild(document.createTextNode(desc[j]));
		}

		controls.appendChild(descdiv);
	}

	if (steps.length != 0)
	{
		var sharelink = document.createElement("a");
		sharelink.href="?" + encode_steps(steps);
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

function ideal_pos()
{
	if (keyframes.length == 1) return keyframes[0];

	return interpolate_position(
		keyframe(frame),
		keyframe(frame + 1), k);
}

function updateDrawnPos()
{
	var therealpos = ideal_pos();

	thepos = (thepos ? interpolate_position(thepos, therealpos, drag) : therealpos);
}

function tick()
{
	if (!sliding && !paused && steps.length != 0)
	{
		k += engine.getDeltaTime() / speed;
		if (k >= 1)
		{
			k -= 1;
			if (k <= 0) k = 0;

			++frame_in_seq;

			var last_frame_in_seq = transitions[steps[seqindex].transition].frames.length - 1;
			if (seqindex == steps.length - 1) last_frame_in_seq += 6;

			if (frame_in_seq >= last_frame_in_seq)
			{
				++seqindex;
				if (seqindex == steps.length)
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

function follow(steps, mirror)
{
	var reo;
	var r = [];

	var cores = v3(0,0,0);

	for (var i = 0; i != steps.length; ++i)
	{
		var step = steps[i];
		var trans = transitions[step.transition];
		var newframes = trans.frames;

		if (r.length != 0) r.pop();

		if (i != 0)
		{
			var prev_step = steps[i - 1];
			reo = compose_reo(
				compose_reo(
					inverse_reo(step_from(step).reo),
					step_to(prev_step).reo),
				reo);
		}
		else
		{
			reo = step_from(step).reo;
			if (mirror) reo.mirror = !reo.mirror;
		}

		if (!step.reverse)
			for (var j = 0; j != newframes.length; ++j)
			{
				var f = apply_reo(reo, newframes[j]);
				r.push(f);
				cores = cores.add(f[0][Core]).add(f[1][Core]);
			}
		else
			for (var j = newframes.length - 1; j >= 0; --j)
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
} // todo: clean up

function on_mirror_button_clicked()
{
	for (var f = 0; f != keyframes.length; ++f)
		mirror(keyframes[f]);
}

function makeScene()
{
	scene = new BABYLON.Scene(engine);
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

	var draw = animated_position_from_array(keyframes[0], scene);

	scene.registerBeforeRender(function(){
			tick();
			if (thepos) draw(thepos);
		});

	engine.runRenderLoop(function(){ scene.render(); });
}

function resetFrames()
{
	if (steps.length == 0)
	{
		keyframes = [nodes[start_node].position];
		if (mirror_view) mirror(keyframes[0]);
	}
	else keyframes = follow(steps, mirror_view);

	document.getElementById("slider").max = keyframes.length - 1;
}

function decode_steps(s)
{
	var r = [];
	var seqs = s.split(',');
	for (var i = 0; i != seqs.length; ++i)
		if (seqs[i][0] == '-')
			r.push({transition: parseInt(seqs[i].substr(1)), reverse: true});
		else
			r.push({transition: parseInt(seqs[i]), reverse: false});
	return r;
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
				steps = [];
			}
			else
			{
				steps = decode_steps(arg);
				start_node = step_from(steps[0]).node;
			}

		}
		else
		{
			steps = random_path(32);
			start_node = step_from(steps[0]).node;
		}

		resetFrames();
		frame = -1;
		k = 0;
		thepos = ideal_pos();

		refreshDrill();
		refreshPreChoices();
		refreshPostChoices();

		canvas = document.getElementById('renderCanvas');
		engine = new BABYLON.Engine(canvas, true);

		makeScene();

		on_view_change();
	});
