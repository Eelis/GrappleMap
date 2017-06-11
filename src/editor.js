      //var statusElement = document.getElementById('status');
      //var progressElement = document.getElementById('progress');
      //var spinnerElement = document.getElementById('spinner');

      var Module = {
        preRun: [],
        postRun: [],
        print: (function() {
          var element = document.getElementById('output');
          if (element) element.value = ''; // clear browser cache
          return function(text) {
            if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
            // These replacements are necessary if you render to raw HTML
            //text = text.replace(/&/g, "&amp;");
            //text = text.replace(/</g, "&lt;");
            //text = text.replace(/>/g, "&gt;");
            //text = text.replace('\n', '<br>', 'g');
            console.log(text);
            if (element) {
              element.value += text + "\n";
              element.scrollTop = element.scrollHeight; // focus on bottom
            }
          };
        })(),
        printErr: function(text) {
          if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
          if (0) { // XXX disabled for safety typeof dump == 'function') {
            dump(text + '\n'); // fast, straight to the real console
          } else {
            console.error(text);
          }
        },
        canvas: (function() {
          var canvas = document.getElementById('canvas');

          // As a default initial behavior, pop up an alert when webgl context is lost. To make your
          // application robust, you may want to override this behavior before shipping!
          // See http://www.khronos.org/registry/webgl/specs/latest/1.0/#5.15.2
          canvas.addEventListener("webglcontextlost", function(e) { alert('WebGL context lost. You will need to reload the page.'); e.preventDefault(); }, false);

          return canvas;
        })(),
        setStatus: function(text) {
        /*
          if (!Module.setStatus.last) Module.setStatus.last = { time: Date.now(), text: '' };
          if (text === Module.setStatus.text) return;
          var m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
          var now = Date.now();
          if (m && now - Date.now() < 30) return; // if this is a progress update, skip it if too soon
          if (m) {
            text = m[1];
            progressElement.value = parseInt(m[2])*100;
            progressElement.max = parseInt(m[4])*100;
            progressElement.hidden = false;
            spinnerElement.hidden = false;
          } else {
            progressElement.value = null;
            progressElement.max = null;
            progressElement.hidden = true;
            if (!text) spinnerElement.style.display = 'none';
          }
          //statusElement.innerHTML = text;
          */
        },
        totalDependencies: 0,
        monitorRunDependencies: function(left) {
          this.totalDependencies = Math.max(this.totalDependencies, left);
          //Module.setStatus(left ? 'Preparing... (' + (this.totalDependencies-left) + '/' + this.totalDependencies + ')' : 'All downloads complete.');
        }
      };
      Module.setStatus('Downloading...');
      window.onerror = function(event) {
        // TODO: do not warn on ok events like simulating an infinite loop or exitStatus
        //Module.setStatus('Exception thrown, see JavaScript console');
        //spinnerElement.style.display = 'none';
        Module.setStatus = function(text) {
          if (text) Module.printErr('[post-exception status] ' + text);
        };
      };

/*
	var memoryInitializer = 'grapplemap_editor.js.mem';

	if (typeof Module['locateFile'] === 'function')
	{
		memoryInitializer = Module['locateFile'](memoryInitializer);
	}
	else if (Module['memoryInitializerPrefixURL'])
	{
		memoryInitializer = Module['memoryInitializerPrefixURL'] + memoryInitializer;
	}
	var meminitXHR = Module['memoryInitializerRequest'] = new XMLHttpRequest();
	meminitXHR.open('GET', memoryInitializer, true);
	meminitXHR.responseType = 'arraybuffer';
	meminitXHR.send(null);
*/

var browse_controls = document.getElementById('browse_controls');
var playback_controls = document.getElementById('playback_controls');
var edit_controls = document.getElementById('edit_controls');
var selection_body = document.getElementById('selection_body');
var the_selection;

function loadDB(f)
{
	var reader = new FileReader();

	reader.onload = function(e) { Module.loadDB(e.target.result); };

	reader.readAsArrayBuffer(f);
}

function gui_command(cmd)
{
	Module.gui_command(cmd);
}

function mode_change(mode)
{
	browse_controls.style.display = (mode == 'browse' ? 'table' : 'none');
	playback_controls.style.display = (mode == 'playback' ? 'table' : 'none');
	edit_controls.style.display = (mode == 'edit' ? 'table' : 'none');

	gui_command('mode ' + mode);
}

function sync_resolution()
{
	gui_command("resolution " + canvas.clientWidth + " " + canvas.clientHeight);
	setTimeout(sync_resolution, 1000);
}

setTimeout(sync_resolution, 1000);

function gui_commander(cmd)
{
	return function(){ gui_command(cmd); };
}

function browseto()
{
	var desc = prompt(
		"Specify one of:\n" +
		"- a position (e.g. 'p34')\n" +
		"- a transition (e.g. 't1383')\n" +
		"- a path (e.g. '39,23,45')");

	if (desc != null) gui_command("browseto " + desc);
}

var changed_things = [];

function is_dirty()
{
	return changed_things.length != 0;
}

function update_modified(nodes, edges)
{
	changed_things = [];
	nodes.forEach(function(n){ changed_things.push('p' + n); });
	edges.forEach(function(e){ changed_things.push('t' + e); });

	var div = document.getElementById('info_modified');
	div.innerHTML = "";
	div.appendChild(document.createTextNode("Modified: " + changed_things.join(", ")));
	div.style.display = is_dirty() ? 'block' : 'none';
}

function highlight_segment(seq, seg, pos)
{
	the_selection.forEach(function(selseq, i)
		{
			if (selseq.id == seq)
				document.getElementById('metadata').value
					= selseq.description.join("\n");

			selseq.segment_indicators.forEach(function(indicators, selseg)
				{
					indicators.forEach(function(ind)
						{
							ind.style.color =
								(selseq.id == seq && seg == selseg && pos == -1
									? 'lime'
									: 'black');
						});
				});

			selseq.position_indicators.forEach(function(indicator, p)
				{
					var hl = selseq.id == seq && pos == p;

					if (i != 0)
					{
						var prev = the_selection[i-1];
						hl = hl || (prev.id == seq && p == 0 && pos == prev.frames.length - 1);
					}

					indicator.style.color = (hl ? 'lime' : 'black');
				});
		});
}

function add_pos_indicator(seqindex, posnum, islast)
{
	var seq = the_selection[seqindex];

	var t = document.createElement("a");
	t.className = "position_indicator";
	t.text = (posnum == 0 || islast ? "█" : "o");
	t.title = "position " + posnum;

	if (posnum == 0)
		t.title += " (= node " + seq.from.node + ")";
	else if (islast)
		t.title += " (= node " + seq.to.node + ")";

	t.onmouseover = (function(x, y)
		{
			return function()
				{
					gui_command("goto_position " + x + " " + y);
					highlight_segment(x, y, y);
				};
		})(seq.id, posnum);

	seq.position_indicators[posnum] = t;
	return t;
}

function add_seg_indicator(seqindex, segnum, txt)
{
	var seq = the_selection[seqindex];

	var t = document.createElement("a");
	t.className = "segment_indicator";
	t.text = txt;
	t.title = "segment " + segnum;
	t.onmouseover = (function(x, y)
		{
			return function()
				{
					gui_command("goto_segment " + x + " " + y);
					highlight_segment(x, y, -1);
				};
		})(seq.id, segnum);

	if (seq.segment_indicators[segnum] == null)
		seq.segment_indicators[segnum] = [];

	seq.segment_indicators[segnum].push(t);

	return t;
}

function make_transition_cell(i)
{
	var seq = the_selection[i];
	var desc = seq.description;
	var cell = document.createElement("td");

	{
		var t = document.createElement("a");
		t.text = (i + 1) + ". " + desc[0];
		t.title = "t" + seq.id;
		for (var j = 1; j < desc.length; ++j)
			t.title += '\n' + desc[j];
		cell.appendChild(t);
	}

	if (the_selection.length != 1 && (i == 0 || i == the_selection.length - 1)) // add remove button
	{
		var btn = document.createElement("button");
		btn.style.marginLeft = "0.7em";
		btn.appendChild(document.createTextNode("x"));
		btn.onclick = gui_commander("set_selected " + seq.id);
		cell.appendChild(btn);
	}

	return cell;
}

function set_selection(sel, post_choices, pre_choices)
{
	the_selection = sel;

	selection_body.innerHTML = '';

	var table = document.createElement("table");
	table.className = "pathTable";

	pre_choices.forEach(function(c)
		{
			var btn = document.createElement("button");
			btn.style.width = '55%';
			btn.title = "from " + c[2];
			btn.appendChild(document.createTextNode(c[1][0]));
			btn.addEventListener("click", (function(sn)
				{ return function(){ gui_command("set_selected " + sn + " true"); }; })(c[0]));
			selection_body.appendChild(btn);
			selection_body.appendChild(document.createElement("br"));
		});

	// first row

	{
		var pre = document.createElement("pre");
		pre.appendChild(document.createTextNode("┃"));

		var cell = document.createElement("td");
		cell.appendChild(pre);

		var row = document.createElement("tr");
		row.appendChild(cell);
		row.appendChild(document.createElement("td"));

		table.appendChild(row);
	}

	// transition rows

	for (var i = 0; i != sel.length; ++i)
	{
		var seq = sel[i];
		var desc = seq.description;

		seq.segment_indicators = [];
		seq.position_indicators = [];

		var hyphen = "━";

		var pre = document.createElement("pre");

		if (seq.frames.length == 2)
		{
			pre.appendChild(add_pos_indicator(i, 0, false));
			pre.appendChild(document.createTextNode("\n"));
			pre.appendChild(add_seg_indicator(i, 0, "┃"));
		}
		else
		{
			var width = Math.floor(seq.frames.length / 2);

			pre.appendChild(add_seg_indicator(i, width - 1, "┏" + (seq.frames.length % 2 == 1 ? hyphen : "")));

			for (var j = width - 1; j >= 0; --j)
			{
				pre.appendChild(add_pos_indicator(i, j, false));

				if (j != 0)
					pre.appendChild(add_seg_indicator(i, j - 1, hyphen));
			}

			pre.appendChild(document.createTextNode("\n"));
			pre.appendChild(add_seg_indicator(i, width - 1, "┗" + (seq.frames.length % 2 == 1 ? "" : hyphen)));

			for (var j = width; j < seq.frames.length - 1; ++j)
			{
				pre.appendChild(add_pos_indicator(i, j, false));
				if (j < seq.frames.length - 2)
					pre.appendChild(add_seg_indicator(i, j, hyphen));
			}

			pre.appendChild(add_seg_indicator(i, seq.frames.length - 2, "┓"));
		}

		var cell = document.createElement("td");
		cell.appendChild(pre);

		var row = document.createElement("tr");
		row.appendChild(cell);
		row.appendChild(make_transition_cell(i));

		table.appendChild(row);
	}

	// last row

	{
		var pre = document.createElement("pre");
		pre.appendChild(add_pos_indicator(sel.length - 1, sel[sel.length - 1].frames.length - 1, true));
		pre.appendChild(document.createTextNode("\n┃"));

		var cell0 = document.createElement("td");
		cell0.appendChild(pre);

		var row = document.createElement("tr");
		row.appendChild(cell0);

		table.appendChild(row);
	}

	selection_body.appendChild(table);

	post_choices.forEach(function(c)
		{
			var btn = document.createElement("button");
			btn.style.width = '55%';
			btn.title = "to " + c[2];
			btn.appendChild(document.createTextNode(c[1][0]));
			btn.addEventListener("click", (function(sn)
				{ return function(){ gui_command("set_selected " + sn + " true"); }; })(c[0]));
			selection_body.appendChild(btn);
			selection_body.appendChild(document.createElement("br"));
		});
}

function prepend_new()
{
	var destination = prompt("Source position? (e.g. '34')");
	if (destination != null) gui_command("prepend_new " + destination);
}

function append_new()
{
	var destination = prompt("Destination position? (e.g. '34')");
	if (destination != null) gui_command("append_new " + destination);
}

function v3(x,y,z) { return 0; }

/*
	var myblob = new Blob(["dinghmz"], {type:"text/plain"});

	var fileURL = URL.createObjectURL(myblob);
    var a         = document.createElement('a');
	a.href        = fileURL; 
	a.text        = "Download DB";
	document.body.appendChild(a);
*/

window.addEventListener("beforeunload", function(e)
	{
		if (!is_dirty()) return undefined;

		var confirmationMessage =
			'It looks like you have been editing. ' +
			'If you leave before saving, your changes will be lost.';

		(e || window.event).returnValue = confirmationMessage; // Gecko + IE
		return confirmationMessage; // Gecko + Webkit, Safari, Chrome etc.
	});
