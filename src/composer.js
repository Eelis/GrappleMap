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
var frame = 0;
var drag = 0.20;
var k = 0;

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

function drillButton(textA, textB, onclick)
{
	var btn = document.createElement("button");
	btn.style.textAlign = "left";
	btn.style.width = "100%";
	btn.appendChild(document.createTextNode(textA));
	btn.appendChild(document.createElement("br"));

	var table = document.createElement("table");
	var row = document.createElement("tr");

	var cell = document.createElement("td");
	cell.style.paddingLeft = "15px";
	cell.style.verticalAlign = "top";
	cell.appendChild(document.createTextNode("↳"));
	row.appendChild(cell);

	cell = document.createElement("td");
	cell.appendChild(document.createTextNode(textB));
	row.appendChild(cell);
	table.appendChild(row);

	btn.appendChild(table);

	btn.addEventListener("click", onclick);
	
	return btn;
}

function refreshPreChoices()
{
	var elem = document.getElementById("pre_choices");
	elem.innerHTML = "";

	var choices = nodes[start_node].incoming;
	document.getElementById("pre_choices_label").style.display =
		choices.length == 0 ? 'none' : 'inline';

	choices.forEach(function(step)
		{
			elem.appendChild(drillButton(
				nodes[step_from(step).node].description,
				transitions[step.transition].description[0],
				function(c){ return function(){
					steps = [c].concat(steps);
					start_node = step_from(c).node;
					resetFrames();
					frame = -1;
					k = 0;
					frame_in_seq = -1;
					thepos = ideal_pos();
					refreshDrill();
					refreshPreChoices();
				}}(step)));
		});
}

function refreshPostChoices()
{
	var elem = document.getElementById("post_choices");
	elem.innerHTML = "";

	var end_node = (steps.length == 0)
		? start_node
		: step_to(steps[steps.length - 1]).node;

	var choices = nodes[end_node].outgoing;
	document.getElementById("post_choices_label").style.display =
		choices.length == 0 ? 'none' : 'inline';

	choices.forEach(function(step)
		{
			elem.appendChild(drillButton(
				transitions[step.transition].description[0],
				nodes[step_to(step).node].description,
				function(c){ return function(){
					steps.push(c);
					resetFrames();
					refreshDrill();
					refreshPostChoices();
				}}(step)));
		});
}

function node_link(node)
{
	var link = document.createElement("a");
	link.href = "../position/" + node + "e.html";
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

	var info_mode = document.getElementById('info_mode_checkbox').checked;

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

		if (info_mode)
		{
			var descdiv = document.createElement("div");
			descdiv.style.marginLeft = "100px";
			descdiv.style.marginTop = "0px";

			descdiv.appendChild(document.createTextNode("transition " + seq));
			if (transitions[seq].line_nr)
				descdiv.appendChild(document.createTextNode(" @ line " + transitions[seq].line_nr));
			descdiv.appendChild(document.createElement("br"));

			for (var j = 1; j <= desc.length - 1; ++j)
			{
				if (j != 1) descdiv.appendChild(document.createElement("br"));
				descdiv.appendChild(document.createTextNode(desc[j]));
			}

			controls.appendChild(descdiv);
		}
	}

	try
	{
		history.replaceState(null, "", "index.html?" +
			(steps.length != 0
				? encode_steps(steps)
				: "p" + start_node));
	}
	catch (e) {}
		// When browsing locally, the replaceState above
		// throws a security exception with Chrome.
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

function keyframe(i)
{
	return keyframes[bound_frame_index(i, keyframes.length)];
}

function on_view_change()
{
	var vv = document.getElementById("view_select").value;

	scene.activeCamera = (vv == "external" ? externalCamera : firstPersonCamera);
}

function on_info_change()
{
	var info_mode = document.getElementById('info_mode_checkbox').checked;

	var panel = document.getElementById('panel');

	panel.style.width = (info_mode ? '50%' : '30%');

	document.getElementById('renderCanvas').style.width = (info_mode ? '50%' : '70%');

	refreshDrill();

	engine.resize();
}

function updateCamera()
{
	var vv = document.getElementById("view_select").value;
	if (vv == "external") return;
	var pl = parseInt(vv);

	firstPersonCamera.upVector = thepos[pl][Head].subtract(thepos[pl][Neck]);

	firstPersonCamera.setTarget(
		thepos[pl][LeftFingers]
		.add(thepos[pl][RightFingers])
		.scale(0.5));

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

	thepos = (thepos ? interpolate_position(thepos, therealpos, sliding ? 0.12 : drag) : therealpos);
}

function tick()
{
	if (!sliding && !paused && steps.length != 0)
	{
		k += Math.min(engine.getDeltaTime() / speed, 2);
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
			reo = copy_reo(step_from(step).reo);

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
	mirror_view = !mirror_view;
	for (var f = 0; f != keyframes.length; ++f)
		mirror(keyframes[f]);
}

function resetFrames()
{
	if (steps.length == 0)
	{
		var p = nodes[start_node].position;

		var r = {
				mirror: mirror_view,
				swap_players: false,
				angle: 0,
				offset: p[0][Core].add(p[1][Core]).scale(-0.5)
			};
		r.offset.y = 0;

		keyframes = [apply_reo(r, p)];
	}
	else keyframes = follow(steps, mirror_view);

	document.getElementById("slider").max = keyframes.length - 1;

	var dianodes=[];
	steps.forEach(function(s){
			dianodes.push(transitions[s.transition].from.node);
			dianodes.push(transitions[s.transition].to.node);
		});

	if (steps.length == 0) dianodes.push(start_node);

	document.getElementById('explorer_link').href =
		"../explorer/index.html?" + dianodes.join(",");
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

function random_drill()
{
	steps = random_path(32);
	start_node = step_from(steps[0]).node;

	resetFrames();
	frame = -1;
	seqindex = 0;
	frame_in_seq = -5;
	k = 0;
	thepos = ideal_pos();

	refreshDrill();
	refreshPreChoices();
	refreshPostChoices();
	pick_bullet();
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

		makeScene(thepos);

		on_view_change();
		
		window.addEventListener('resize', function() { engine.resize(); });
	});
