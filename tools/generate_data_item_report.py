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
#report.write('        <meta name="viewport" content="width=device-width, user-scalable=no, minimum-scale=1.0, maximum-scale=1.0">\n')
report.write('        <meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no">\n')
report.write('        <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/css/bootstrap.min.css" integrity="sha384-Gn5384xqQ1aoWXA+058RXPxPg6fy4IWvTNh0E263XmFcJlSAwiGgFAW/dAiS6JXm" crossorigin="anonymous">\n')
report.write('        <script src="https://code.jquery.com/jquery-3.2.1.slim.min.js" integrity="sha384-KJ3o2DKtIkvYIK3UENzmM7KCkRr/rE9/Qpg6aAZGJwFDMVNA/GpGFF93hXpG5KkN" crossorigin="anonymous"></script>\n')
report.write('        <script src="https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.12.9/umd/popper.min.js" integrity="sha384-ApNbgh9B+Y1QKtv3Rn7W3mgPxhU9K/ScQsAP7hUibX39j7fakFPskvXusvfa0b4Q" crossorigin="anonymous"></script>\n')
report.write('        <script src="https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/js/bootstrap.min.js" integrity="sha384-JZR6Spejh4U02d8jOt6vLEHfe/JQGiRRSQQxSfFWpi1MquVdAyjUar5+76PVCmYl" crossorigin="anonymous"></script>\n')
report.write('        <script src="https://cdn.plot.ly/plotly-latest.min.js"></script>\n')
report.write('        <style>\n')
report.write('        body {\n')
report.write('          font-size: .875rem;\n')
report.write('        }\n')
report.write('        \n')
report.write('        .feather {\n')
report.write('          width: 16px;\n')
report.write('          height: 16px;\n')
report.write('          vertical-align: text-bottom;\n')
report.write('        }\n')
report.write('        \n')
report.write('        /*\n')
report.write('         * Sidebar\n')
report.write('         */\n')
report.write('        \n')
report.write('        .sidebar {\n')
report.write('          position: fixed;\n')
report.write('          top: 0;\n')
report.write('          bottom: 0;\n')
report.write('          left: 0;\n')
report.write('          z-index: 100; /* Behind the navbar */\n')
report.write('          padding: 0;\n')
report.write('        }\n')
report.write('        \n')
report.write('        .sidebar-sticky {\n')
report.write('          position: -webkit-sticky;\n')
report.write('          position: sticky;\n')
report.write('          top: 48px; /* Height of navbar */\n')
report.write('          height: calc(100vh - 48px);\n')
report.write('          padding-top: 1rem;\n')
report.write('          overflow-x: hidden;\n')
report.write('          overflow-y: auto; /* Scrollable contents if viewport is shorter than content. */\n')
report.write('        }\n')
report.write('        \n')
report.write('        .sidebar .nav-link {\n')
report.write('          margin-left: 1rem;\n')
report.write('          font-weight: 500;\n')
report.write('          color: #333;\n')
report.write('        }\n')
report.write('        \n')
report.write('        .sidebar .nav-link .feather {\n')
report.write('          margin-right: 4px;\n')
report.write('          color: #999;\n')
report.write('        }\n')
report.write('        \n')
report.write('        .sidebar .nav-link.active {\n')
report.write('          color: #007bff;\n')
report.write('        }\n')
report.write('        \n')
report.write('        .sidebar .nav-link:hover .feather,\n')
report.write('        .sidebar .nav-link.active .feather {\n')
report.write('          color: inherit;\n')
report.write('        }\n')
report.write('        \n')
report.write('        .sidebar-heading {\n')
report.write('          font-size: .75rem\n');
report.write('          text-transform: uppercase;\n')
report.write('        }\n')
report.write('        \n')
report.write('        /*\n')
report.write('         * Navbar\n')
report.write('         */\n')
report.write('        \n')
report.write('        </style>\n')
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
                report.write('                                    \'mode\': \'%s\',\n' % req[1])
            report.write('                                }\n')
            report.write('                            },\n')
        report.write('                            ],\n')
        report.write('                        },\n')
    report.write('                        ],\n')
    report.write('                    },\n')
report.write('            };\n')
report.write('            function plot_cube(begin, end, d, n, w)\n')
report.write('            {\n')
report.write('                var t = \'scatter3d\';\n')
report.write('                if (begin.length == 2)\n')
report.write('                {\n')
report.write('                    t = \'scatter\';\n')
report.write('                    begin = [begin[0], begin[1]]\n')
report.write('                    end = [end[0], end[1]]\n')
report.write('                }\n')
report.write('                return {\n')
report.write('                        type : t,\n')
report.write('                        name : n,\n')
report.write('                        mode: \'lines\',\n')
report.write('                        line: {\n')
report.write('                            width: w,\n')
report.write('                            dash: d,\n')
report.write('                        },\n')
report.write('                        x : [begin[0],   end[0],   end[0], begin[0], begin[0], begin[0], begin[0], begin[0], begin[0], end[0],   end[0], end[0],   end[0],   end[0],   end[0], begin[0]],\n')
report.write('                        y : [begin[1], begin[1],   end[1],   end[1], begin[1], begin[1],   end[1],   end[1],   end[1], end[1],   end[1], end[1], begin[1], begin[1], begin[1], begin[1]],\n')
report.write('                        z : [begin[2], begin[2], begin[2], begin[2], begin[2],   end[2],   end[2], begin[2],   end[2], end[2], begin[2], end[2],   end[2], begin[2],   end[2],   end[2]],\n')
report.write('                };\n')
report.write('                return data;\n')
report.write('            }\n')
report.write('            function plot(data_item)\n')
report.write('            {\n')
report.write('                var x = document.getElementById(\'work_item\');\n')
report.write('                var l = document.getElementById(\'wi_id\');\n')
report.write('                var wi = x.value;\n')
report.write('                var wi_id = l.value;\n')
report.write('                var cur_level;\n')
report.write('                var levels = work_items[wi][\'levels\'];\n')
report.write('                levels.some((level) => {\n')
report.write('                    cur_level = level;\n')
report.write('                    return level[\'id\'] == wi_id;\n')
report.write('                });\n')
report.write('                var traces = [];\n')
report.write('                for (var loc in fragments[data_item])\n')
report.write('                {\n')
report.write('                    fragments[data_item][loc].forEach((info) => {\n')
report.write('                        info[\'ranges\'].forEach((range) => {\n')
report.write('                            traces.push(plot_cube(range[0], range[1], \'dash\', info[\'name\'] + \' (\'+ info[\'mode\'] + \') at \' + loc, \'2\'));\n')
report.write('                        });\n')
report.write('                    });\n')
report.write('                }\n')
report.write('                cur_level[\'refs\'].forEach((ref) => {;\n')
report.write('                    if (ref[\'ref\'] == data_item)\n');
report.write('                    {\n');
report.write('                        ref[\'reqs\'][\'ranges\'].forEach((range) => {\n');
report.write('                            traces.push(plot_cube(range[0], range[1], \'solid\', ref[\'reqs\'][\'mode\'], \'4\'));\n')
report.write('                        });\n');
report.write('                    }\n');
report.write('                });\n')
report.write('                Plotly.newPlot(\'plot\', traces,\n')
report.write('                    {\n')
report.write('                        width: window.width, height: window.height,\n')
report.write('                        margin: { l:10, r:0, b:0, t:30}\n')
report.write('                    },\n')
report.write('                    {\n')
report.write('                        modeBarButtonsToRemove: [\'toImage\'],\n')
report.write('                        modeBarButtonsToAdd:\n')
report.write('                        [{\n')
report.write('                            name: \'toSVG\',\n')
report.write('                            icon: Plotly.Icons.camera,\n')
report.write('                            click: function(gd) {\n')
report.write('                                Plotly.downloadImage(gd, {format: \'svg\'});\n')
report.write('                            }\n')
report.write('                        }]\n')
report.write('                    }\n')
report.write('                );\n')
report.write('            }\n')
report.write('            function list_dis(change_ids)\n')
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
report.write('                var requests_list = document.getElementById(\'requests_list\');\n')
report.write('                while(requests_list.firstChild)\n')
report.write('                    requests_list.removeChild(requests_list.firstChild);\n')
report.write('                levels.forEach((level) => {\n')
report.write('                    if (level[\'id\'] == wi_id)\n')
report.write('                    {\n')
report.write('                        level[\'refs\'].forEach((ref) => {\n')
report.write('                            var item = document.createElement(\'li\');\n')
report.write('                            item.className = "nav-item"\n')
report.write('                            var ref_item = document.createElement(\'button\')\n')
report.write('                            ref_item.className = "nav-link btn"\n')
report.write('                            ref_item.innerHTML = \'Data Item \' + ref[\'ref\'] + \':\'\n')
report.write('                            ref_item.onclick = function(){plot(ref[\'ref\']);};\n')
report.write('                            \n')
report.write('                            \n')
report.write('                            item.appendChild(ref_item)\n')
report.write('                            var list = document.createElement(\'ul\')\n')
report.write('                            list.className = "nav-item"\n')
report.write('                            var list_item = document.createElement(\'li\')\n')
report.write('                            list_item.className = "nav-item"\n')
report.write('                            list_item.innerHTML = \'Access mode: \' + ref[\'reqs\'][\'mode\'];\n')
report.write('                            list.appendChild(list_item)\n')
report.write('                            var list_item = document.createElement(\'li\')\n')
report.write('                            list_item.className = "nav-item"\n')
report.write('                            list_item.innerHTML = \'Ranges: <br\>\' + JSON.stringify(ref[\'reqs\'][\'ranges\']);\n')
report.write('                            list.appendChild(list_item)\n')
report.write('                            item.appendChild(list)\n')
report.write('                            requests_list.appendChild(item);\n')
report.write('                        });\n')
report.write('                    }\n')
report.write('                });\n')
report.write('            }\n')
report.write('        </script>\n')
report.write('    </head>\n')
report.write('    <body>\n')
report.write('        <nav class="navbar navbar-expand-lg navbar-light bg-light sticky-top">\n')
report.write('           <a class="navbar-brand" href="#">\n')
report.write('               AllScale Data Item Report\n')
report.write('           </a>\n')
report.write('           <div class="collapse navbar-collapse" id="navbarSupportedContent">\n')
report.write('               <ul class="navbar-nav mr-auto">\n')

report.write('                   <li class="nav-item">\n')
report.write('                       <select id="work_item" onchange=list_dis(true) class="form-control">\n')
for wi_name in work_items:
    report.write('                        <option>\n')
    report.write('                            %s\n' % wi_name)
    report.write('                        </option>\n')
report.write('                        </select>\n')
report.write('                    </li>\n')

report.write('                    <li class="nav-item"><span class="nav-link disabled">ID:</span></li>\n')
report.write('                    <li class="nav-item">\n')
report.write('                        <select id="wi_id" onchange=list_dis(false) class="form-control"></select>')
report.write('                    </li>\n')
report.write('                </ul>\n')
report.write('            </div>\n')
report.write('        </nav>\n')
report.write('        <div class="container-fluid">\n')
report.write('            <div class="row">\n')
report.write('                <nav class="col-md-2 d-none d-md-block bg-light sidebar">\n')
report.write('                    <div class="sidebar-sticky">\n')
report.write('                        <ul id="requests_list" class="nav flex-column">\n')
report.write('                        </ul>\n')
report.write('                    </div>\n')
report.write('                </nav>\n')
report.write('                <main role="main"  class="col-md-9 ml-sm-auto col-lg-10 pt-3 px-4 center">\n')
report.write('                    <div id="plot"></div>\n')
report.write('                </main>\n')
report.write('            </div>\n')
report.write('        </div>\n')
report.write('        <script>list_dis(true)</script>\n')
report.write('    </body>\n')
report.write('</html>\n')
