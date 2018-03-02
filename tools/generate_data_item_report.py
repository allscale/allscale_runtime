#!env python

import sys
from os import listdir
import collections
import math

report_name = 'data_item_report.html'
if len(sys.argv) == 2:
    report_name = sys.argv[1]

fragments = {}
work_items = {}

for f in listdir('.'):
    if f.startswith('data_item') and f.endswith('.log'):
        loc = f.split('.')[1]
        for line in open(f):
            line = line[:-1]
            info, region = line.split(':')
            mode, name, ref = [s.strip(' ') for s in info.split(',')]
            ranges = []
            i = 0
            begin = None
            while i < len(region):
                if region[i] == '[':
                    if region[i+1] == '[':
                        i = i + 1
                    end = region.find(']', i)
                    point = [int(p) for p in region[i+1:end].split(',')]
                    if begin is None:
                        begin = point
                    else:
                        ranges.append([begin, point])
                        begin = None
                    i = end
                else:
                    i = i +1

            if mode.startswith('create'):
                if not ref in fragments:
                    fragments[ref] = {}
                if not loc in fragments[ref]:
                    fragments[ref][loc] = []
                wi_name = name[:(name.find(')') + 1)]
                fragments[ref][loc].append((ranges, mode.split('(')[1][:-1], wi_name))

            if mode.startswith('require'):
                name = name[:(name.find(')') + 1)]
                wi_name, wid = name.split('(')
                wid = wid[:-1]
                if not wi_name in work_items:
                    work_items[wi_name] = {'levels': collections.OrderedDict(), 'refs': []}
                if not wid in work_items[wi_name]['levels']:
                    work_items[wi_name]['levels'][wid] = {}

                if not ref in work_items[wi_name]['refs']:
                    work_items[wi_name]['refs'].append(ref)
                if not ref in work_items[wi_name]['levels'][wid]:
                    work_items[wi_name]['levels'][wid][ref] = []
                work_items[wi_name]['levels'][wid][ref].append((ranges, mode.split('(')[1][:-1]));


report = open(report_name, 'w')

report.write('<!DOCTYPE html>\n')
report.write('<html lang="en">\n')
report.write('    <head>\n')
report.write('        <title>AllScale Data Item Visualization</title>\n')
report.write('        <meta charset="utf-8">\n')
report.write('        <meta name="viewport" content="width=device-width, user-scalable=no, minimum-scale=1.0, maximum-scale=1.0">\n')
report.write('        <script src="https://cdn.plot.ly/plotly-latest.min.js"></script>\n')
report.write('        <script>\n')
report.write('            fragments = {\n')
for ref in fragments:
    report.write('                \'%s\' : \n' % ref)
    report.write('                    {\n')
    for loc in fragments[ref]:
        report.write('                        %s :\n' % loc)
        report.write('                            [\n')
        for r in fragments[ref][loc]:
            report.write('                                {\n')
            report.write('                                    \'ranges\': %s,\n' % r[0])
            report.write('                                    \'mode\': \'%s\',\n' % r[1])
            report.write('                                    \'name\': \'%s\'\n' % r[2])
            report.write('                                },\n')
        report.write('                            ],\n')
    report.write('                    },\n')

report.write('            };\n')
report.write('            work_items = {\n')
for wi in work_items:
    report.write('                \'%s\' : \n' % wi)
    report.write('                    {\n')
    report.write('                        \'refs\' : %s,\n' % work_items[wi]['refs'])
    first_level = None
    iterations = 0
    report.write('                        \'levels\' : [\n')
    for level in work_items[wi]['levels']:
        if first_level is None:
            first_level = level
        if len(first_level.split('.')) == len(level.split('.')):
            iterations = iterations + 1
        report.write('                        {\n')
        report.write('                            \'id\': \'%s\',\n' % level)
        report.write('                            \'refs\':\n')
        report.write('                            [\n')
        for ref in work_items[wi]['levels'][level]:
            reqs = work_items[wi]['levels'][level][ref]
            report.write('                            {\n')
            report.write('                                \'ref\': \'%s\',\n' % ref)
            report.write('                                \'reqs\':\n')
            report.write('                                {\n')
            for req in reqs:
                report.write('                                    \'ranges\': %s,\n' % req[0])
                report.write('                                    \'mode\': \'%s\'\n' % req[1])
            report.write('                                }\n')
            report.write('                            },\n')
        report.write('                            ],\n')
        report.write('                        },\n')
    report.write('                        ],\n')
    report.write('                        \'iterations\' : %s,\n' % iterations)
    num = len(work_items[wi]['levels'])/iterations
    report.write('                        \'level_depth\' : %s\n' % int(math.log(num + 1)/ math.log(2)))
    report.write('                    },\n')
report.write('            };\n')
report.write('            function plot_cube(begin, end, d, n)\n')
report.write('            {\n')
report.write('                return {\n')
report.write('                        type : \'scatter3d\',\n')
report.write('                        name : n,\n')
report.write('                        mode: \'lines\',\n')
report.write('                        line: {\n')
report.write('                            width: \'3\',\n')
report.write('                            dash: d,\n')
report.write('                        },\n')
report.write('                        x : [begin[0],   end[0],   end[0], begin[0], begin[0], begin[0], begin[0], begin[0], begin[0], end[0],   end[0], end[0],   end[0],   end[0],   end[0], begin[0]],\n')
report.write('                        y : [begin[1], begin[1],   end[1],   end[1], begin[1], begin[1],   end[1],   end[1],   end[1], end[1],   end[1], end[1], begin[1], begin[1], begin[1], begin[1]],\n')
report.write('                        z : [begin[2], begin[2], begin[2], begin[2], begin[2],   end[2],   end[2], begin[2],   end[2], end[2], begin[2], end[2],   end[2], begin[2],   end[2],   end[2]],\n')
report.write('                };\n')
report.write('                return data;\n')
report.write('            }\n')
report.write('            function plot(change_ids)\n')
report.write('            {\n')
report.write('                var x = document.getElementById(\'work_item\');\n')
report.write('                var l = document.getElementById(\'wi_id\');\n')
report.write('                var levels = work_items[x.value][\'levels\'];\n')
report.write('                if (change_ids == true)\n')
report.write('                {\n')
report.write('                    l.value = levels[0][\'id\'];\n')
report.write('                    while(l.firstChild)\n')
report.write('                        l.removeChild(l.firstChild);\n')
report.write('                    levels.forEach((level) => {\n')
report.write('                        var opt = document.createElement(\'option\');\n')
report.write('                        opt.value = level[\'id\'];\n')
report.write('                        opt.innerHTML = level[\'id\'];\n')
report.write('                        l.appendChild(opt);\n')
report.write('                    });\n')
report.write('                }\n')
report.write('                var wi_id = l.value;\n')
report.write('                console.log(wi_id);\n')
report.write('                var content = \'\';\n')
report.write('                content += \'<p>\';\n')
report.write('                content += \'<ul>\';\n')
report.write('                content += \'    <li>\';\n')
report.write('                content += \'    Requests:\';\n')
report.write('                content += \'    <ul>\';\n')
report.write('                var cur_level;\n')
report.write('                levels.forEach((level) => {\n')
report.write('                    if (level[\'id\'] == wi_id)\n')
report.write('                    {\n')
report.write('                        cur_level = level;\n')
report.write('                        level[\'refs\'].forEach((ref) => {\n')
report.write('                            content += \'<li>\'\n')
report.write('                            content += \'Data Item \' + ref[\'ref\'] + \':\'\n')
report.write('                            content += \'<ul>\'\n')
report.write('                            content += \'<li>\'\n')
report.write('                            content += \'Access mode: \' + ref[\'reqs\'][\'mode\'];\n')
report.write('                            content += \'</li>\'\n')
report.write('                            content += \'<li>\'\n')
report.write('                            content += \'Ranges: \' + JSON.stringify(ref[\'reqs\'][\'ranges\']);\n')
report.write('                            content += \'</li>\'\n')
report.write('                            content += \'</ul>\'\n')
report.write('                            content += \'</li>\'\n')
report.write('                        });\n')
report.write('                        \n')
report.write('                    }\n')
report.write('                });\n')
report.write('                content += \'    </ul>\';\n')
report.write('                content += \'    </li>\';\n')
report.write('                content += \'</ul>\';\n')
report.write('                content += \'</p>\';\n')
report.write('                work_items[x.value][\'refs\'].forEach( (ref) => {\n')
report.write('                    content += \'<p>Data Item: \' + ref;\n')
report.write('                    content += \'<div id="plot_\' + ref + \'"></div></p>\';\n')
report.write('                });\n')
report.write('                document.getElementById(\'content\').innerHTML = content;\n')
report.write('                var traces = {};\n')
report.write('                work_items[x.value][\'refs\'].forEach( (ref) => {\n')
report.write('                    for (var loc in fragments[ref])\n')
report.write('                    {\n')
report.write('                        traces[ref] = [];')
report.write('                        fragments[ref][loc].forEach((info) => {\n')
report.write('                            info[\'ranges\'].forEach((range) => {\n')
report.write('                                traces[ref].push(plot_cube(range[0], range[1], \'dash\', info[\'name\'] + \' (\'+ info[\'mode\'] + \') at \' + loc));\n')
report.write('                            \n')
report.write('                            });\n')
report.write('                        });\n')
report.write('                    }\n')
report.write('                    Plotly.newPlot(\'plot_\' + ref, traces);\n')
report.write('                });\n')
report.write('                cur_level[\'refs\'].forEach((ref) => {;\n')
report.write('                    ref[\'reqs\'][\'ranges\'].forEach((range) => {\n');
report.write('                        traces[ref[\'ref\']].push(plot_cube(range[0], range[1], \'solid\', ref[\'reqs\'][\'mode\']));\n')
report.write('                    });\n');
report.write('                    Plotly.newPlot(\'plot_\' + ref[\'ref\'], traces[ref[\'ref\']]);\n')
report.write('                });\n')
report.write('            }\n')
report.write('        </script>\n')
report.write('    </head>\n')
report.write('    <body>\n')

report.write('        <select id="work_item" onchange=plot(true)>\n')
for wi_name in work_items:
    report.write('            <option>\n')
    report.write('                %s\n' % wi_name)
    report.write('            </option>\n')
report.write('        </select>\n')
report.write('        Id: <select id="wi_id" onchange=plot(false)>')
report.write('        </select>\n')
report.write('        <div id="content">0</div>\n')

report.write('        <script>\n')
report.write('            plot(true);\n')
report.write('            function toggle(name)\n')
report.write('            {\n')
report.write('                var x = document.getElementById(name);\n')
report.write('                if (x.style.display === "none") {\n')
report.write('                    x.style.display = "block";\n')
report.write('                } else {\n')
report.write('                    x.style.display = "none";\n')
report.write('                }\n')
report.write('            }\n')

#for ref in refs:
#    report.write('            Plotly.newPlot(\n')
#    report.write('                \'div_%s\',\n'% ref)
#    report.write('                [\n')
#    for ranges in refs[ref]:
#        report.write('                    plot_cube(%s,%s, \'red\', \'partition 0 on locality 0\'),\n' % (ranges[0], ranges[1]))
#    report.write('                ],\n')
#    report.write('                {\n')
#    report.write('                    width: 1000,\n')
#    report.write('                    height: 600,\n')
#    report.write('                    margin: {\n')
#    report.write('                        l: 10,\n')
#    report.write('                        r: 0,\n')
#    report.write('                        b: 0,\n')
#    report.write('                        t: 30,\n')
#    report.write('                    },\n')
#    report.write('                }\n')
#    report.write('            );\n')

report.write('        </script>\n')
report.write('    </body>\n')
report.write('</html>\n')
