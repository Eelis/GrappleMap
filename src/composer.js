var engine;
var scene;
var seqs = [150];
var start_node = null;
var start_bullet = null;
var seq_bullets = [];
var sliding = false;
var seqindex = 0;
var frame_in_seq = 0;

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

function on_pop_front()
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

function on_pop_back()
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
			btn.addEventListener("click", on_pop_front);
			controls.appendChild(btn);
		}
		else if (i == seqs.length - 1/* && seqs.length >= 1*/)
		{
			var btn = document.createElement("button");
			btn.appendChild(document.createTextNode("x"));
			btn.addEventListener("click", on_pop_back);
			controls.appendChild(btn);
		}

		var to_node = transitions[seq].to;

		controls.appendChild(document.createElement("br"));
		controls.appendChild(document.createTextNode("\u00a0 \u00a0 \u00a0 \u00a0 \u00a0 \u00a0 \u00a0 ↳ \u00a0")); // todo
		controls.appendChild(node_link(to_node));
		controls.appendChild(document.createElement("br"));
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

function tick()
{
	if (sliding) return;

	k += engine.getDeltaTime() / speed;
	if (k < 1) return;
	
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

var id_reo =
	{ mirror: false
	, swap_players: false
	, angle: 0
	, offset: v3(0, 0, 0) };

Array.prototype.swap = function (x,y) {
  var b = this[x];
  this[x] = this[y];
  this[y] = b;
  return this;
}

function swapLimbs(p)
{
	p.swap(LeftShoulder, RightShoulder);
	p.swap(LeftElbow, RightElbow);
	p.swap(LeftHand, RightHand);
	p.swap(LeftFingers, RightFingers);
	p.swap(LeftWrist, RightWrist);
	p.swap(LeftAnkle, RightAnkle);
	p.swap(LeftToe, RightToe);
	p.swap(LeftHip, RightHip);
	p.swap(LeftHeel, RightHeel);
	p.swap(LeftKnee, RightKnee);
}

function apply_reo(r, p)
{
	var q = [[],[]];

	for (var pl = 0; pl != 2; ++pl)
	for (var j = 0; j != joints.length; ++j)
		q[pl].push(yrot(r.angle, p[pl][j]).add(r.offset));

	if (r.mirror)
	{
		for (var pl = 0; pl != 2; ++pl)
		for (var j = 0; j != joints.length; ++j)
			q[pl][j].x = -q[pl][j].x;

		swapLimbs(q[0]);
		swapLimbs(q[1]);
	}

	if (r.swap_players) q = [q[1], q[0]];

	return q;
}

function yrot(a, v)
{
	return v3
		( Math.cos(a) * v.x + Math.sin(a) * v.z
		, v.y
		, - Math.sin(a) * v.x + Math.cos(a) * v.z
		);
}

function inverse_reo(x)
{
	var c =
		{ offset: yrot(-x.angle, x.offset.negate())
		, angle: -x.angle
		, mirror: x.mirror
		, swap_players: x.swap_players
		};

	if (x.mirror)
	{
		c.angle = -c.angle;
		c.offset.x = -c.offset.x;
	}

	return c;
}

function compose_reo(a, b)
{
	var c =
		{ mirror: a.mirror != b.mirror
		, swap_players: a.swap_players != b.swap_players
		};

	if (a.mirror)
	{
		var boff = v3(-b.offset.x, b.offset.y, b.offset.z);

		c.angle = a.angle - b.angle;
		c.offset = boff.add(yrot(-b.angle, a.offset));
	}
	else
	{
		c.angle = a.angle + b.angle;
		c.offset = b.offset.add(yrot(b.angle, a.offset));
	}

	return c;
	
}

function follow(seqs)
{
	var reo = id_reo;
	var r = [];

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
			reo = transitions[seqs[i]].reo_from;

		for (var j = 0; j != newframes.length; ++j)
			r.push(apply_reo(reo, newframes[j]));
	}

	return r;
}

function resetScene()
{
	engine.stopRenderLoop();

	if (scene) scene.dispose();

	var keyframes = (seqs.length == 0 ? [nodes[start_node].position] : follow(seqs));

	scene = showpos(keyframes, engine);

	document.getElementById("slider").max = keyframes.length - 1;

	frame = -1;
	k = 0;
	frame_in_seq = -1;

	refreshDrill();
	refreshPreChoices();
	refreshPostChoices();

	if (seqs.length != 0) scene.registerBeforeRender(tick);

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
