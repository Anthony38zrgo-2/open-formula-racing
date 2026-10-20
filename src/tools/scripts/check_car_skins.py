
import os
import glob
from xml.etree.ElementTree import ElementTree
from xml.etree.ElementTree import SubElement
from optparse import OptionParser

# pull in common functions
global options
import check_skins

try:
	import svn
	_has_svn = True
except ImportError:
	_has_svn = False

try:
	from git import *
	_has_pygit = True
except ImportError:
	_has_pygit = False

parser = OptionParser()

parser.set_defaults(cars=".", config=None, run=None, svn=None, git=None, proc=None, all=None)
parser.add_option("-c",  "--cars", dest="cars", help="cars directory")
parser.add_option("-C",  "--config", dest="config", help="path to 'speed-dreams-2' config directory")
parser.add_option("-r",  "--run", dest="run", help="command to run SpeedDreams")
parser.add_option("-p",  "--proc", dest="proc", help="command to process preview images")
parser.add_option("-a",  "--all", action="store_true", dest="all", help="process all previews regardless")

if _has_svn:
	parser.add_option("-s", "--svn", action="store_true", dest="svn", help="report svn version numbers")
if _has_pygit:
	parser.add_option("-g", "--git", action="store_true", dest="git", help="report git verison numbers")

(options, args) = parser.parse_args()

# check each of the cars in turn
for root, _, files in os.walk(options.cars):
	car =  os.path.basename(root)

	for name in files:
		(base, ext) = os.path.splitext(name)
		if base == car and ext == ".xml":
			# Found config file
			print("checking", base)
			print("---")

			path = os.sep.join([options.cars, car])
			model = ".".join([os.sep.join([path, car]), "acc"])

			check_skins.check_car(options, "human", "1", path, base, model)

