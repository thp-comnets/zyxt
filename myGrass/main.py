# -*- coding: utf-8 -*-
import sys
import subprocess
import os
import tempfile
import binascii
import tqdm

def init(outputDir, tempDir, filenames):
    QUIET = True

    # Windows
    grass7path = r'C:\OSGeo4W\apps\grass\grass-7.2.svn'
    grass7bin_win = r'C:\OSGeo4W\bin\grass72svn.bat'
    # Linux
    grass7bin_lin = 'grass72'
    # MacOSX
    grass7bin_mac = '/Applications/GRASS/GRASS-7.1.app/'

    if sys.platform.startswith('linux'):
        grass7bin = grass7bin_lin
    elif sys.platform.startswith('win'):
        grass7bin = grass7bin_win
    else:
        OSError('Platform not configured.')

    startcmd = grass7bin + ' --config path'

    p = subprocess.Popen(startcmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    if p.returncode != 0:
        print >>sys.stderr, 'ERROR: %s' % err
        print >>sys.stderr, "ERROR: Cannot find GRASS GIS 7 start script (%s)" % startcmd
        sys.exit(-1)
    if sys.platform.startswith('linux'):
        gisbase = out.strip('\n')
    elif sys.platform.startswith('win'):
        if out.find("OSGEO4W home is") != -1:
            gisbase = out.strip().split('\n')[1]
        else:
            gisbase = out.strip('\n')
        os.environ['GRASS_SH'] = os.path.join(gisbase, 'msys', 'bin', 'sh.exe')

    # Set GISBASE environment variable
    os.environ['GISBASE'] = gisbase
    # define GRASS-Python environment
    gpydir = os.path.join(gisbase, "etc", "python")
    sys.path.append(gpydir)
    ########
    # define GRASS DATABASE
    if sys.platform.startswith('win'):
        gisdb = os.path.join(os.getenv('APPDATA', 'grassdata'))
    else:
        gisdb = os.path.join(os.getenv('HOME', 'grassdata'))

    # override for now with TEMP dir
    # gisdb = os.path.join(tempfile.gettempdir(), 'grassdata')
    gisdb = os.path.join("/home/cuda/", 'grassdata')
    try:
        os.stat(gisdb)
    except:
        os.mkdir(gisdb)

    # location/mapset: use random names for batch jobs
    string_length = 16
    location = binascii.hexlify(os.urandom(string_length))
    mapset   = 'PERMANENT'
    location_path = os.path.join(gisdb, location)

    # open(outputDir+"/grassLocation.txt", "wb").write(location+"\n")
    open(outputDir+"/"+location, "wb")

    # Create new location (we assume that grass7bin is in the PATH)
    myfile = os.path.join(tempDir,filenames[0]+".hgt") #use only one file as a dummy
    startcmd = grass7bin + ' -c ' + myfile + ' -e ' + location_path
    #  from SHAPE or GeoTIFF file
    # startcmd = grass7bin + ' -c ' +' -e ' + location_path

    print startcmd
    p = subprocess.Popen(startcmd, shell=True,
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    if p.returncode != 0:
        print >>sys.stderr, 'ERROR: %s' % err
        print >>sys.stderr, 'ERROR: Cannot generate location (%s)' % startcmd
        sys.exit(-1)
    else:
        print 'Created location %s' % location_path

    # Now the location with PERMANENT mapset exists.

    ########
    # Now we can use PyGRASS or GRASS Scripting library etc. after
    # having started the session with gsetup.init() etc

    # Set GISDBASE environment variable
    os.environ['GISDBASE'] = gisdb

    # Linux: Set path to GRASS libs (TODO: NEEDED?)
    path = os.getenv('LD_LIBRARY_PATH')
    dir  = os.path.join(gisbase, 'lib')
    if path:
        path = dir + os.pathsep + path
    else:
        path = dir
    os.environ['LD_LIBRARY_PATH'] = path

    # languageQUIET
    os.environ['LANG'] = 'en_US'
    os.environ['LOCALE'] = 'C'

    # Windows: NEEDED?
    #path = os.getenv('PYTHONPATH')
    #dirr = os.path.join(gisbase, 'etc', 'python')
    #if path:
    #    path = dirr + os.pathsep + path
    #else:
    #    path = dirr
    #os.environ['PYTHONPATH'] = path

    import grass.script.setup as gsetup
    import grass.script as gscript
    gsetup.init(gisbase, gisdb, location, mapset)

    # print "Importing srtm files"
    # for file in filenames:
    #     if file is not None:
    #         # file = file + ".hgt"
    #         gscript.run_command('r.in.srtm', flags="1", quiet=QUIET, overwrite=True, input=os.path.join(tempDirForMapTiles,file))


    return gscript

def initWithSpecifiedLocation(location):
    QUIET = True

    # Windows
    grass7path = r'C:\OSGeo4W\apps\grass\grass-7.2.svn'
    grass7bin_win = r'C:\OSGeo4W\bin\grass72svn.bat'
    # Linux
    grass7bin_lin = 'grass72'
    # MacOSX
    grass7bin_mac = '/Applications/GRASS/GRASS-7.1.app/'

    if sys.platform.startswith('linux'):
        grass7bin = grass7bin_lin
    elif sys.platform.startswith('win'):
        grass7bin = grass7bin_win
    else:
        OSError('Platform not configured.')

    startcmd = grass7bin + ' --config path'

    p = subprocess.Popen(startcmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    if p.returncode != 0:
        print >> sys.stderr, 'ERROR: %s' % err
        print >> sys.stderr, "ERROR: Cannot find GRASS GIS 7 start script (%s)" % startcmd
        sys.exit(-1)
    if sys.platform.startswith('linux'):
        gisbase = out.strip('\n')
    elif sys.platform.startswith('win'):
        if out.find("OSGEO4W home is") != -1:
            gisbase = out.strip().split('\n')[1]
        else:
            gisbase = out.strip('\n')
        os.environ['GRASS_SH'] = os.path.join(gisbase, 'msys', 'bin', 'sh.exe')

    # Set GISBASE environment variable
    os.environ['GISBASE'] = gisbase
    # define GRASS-Python environment
    gpydir = os.path.join(gisbase, "etc", "python")
    sys.path.append(gpydir)
    ########
    # define GRASS DATABASE
    if sys.platform.startswith('win'):
        gisdb = os.path.join(os.getenv('APPDATA', 'grassdata'))
    else:
        gisdb = os.path.join(os.getenv('HOME', 'grassdata'))

    # override for now with TEMP dir
    gisdb = os.path.join("/home/cuda/", 'grassdata')
    # gisdb = os.path.join("/media/cuda/MyPassport/", 'grassdata_16jan')

    try:
        os.stat(gisdb)
    except:
        os.mkdir(gisdb)

    string_length = 16
    # location = location  # binascii.hexlify(os.urandom(string_length))
    mapset = 'PERMANENT'
    location_path = os.path.join(gisdb, location)

    os.environ['GISDBASE'] = gisdb

    # Linux: Set path to GRASS libs (TODO: NEEDED?)
    path = os.getenv('LD_LIBRARY_PATH')
    dir = os.path.join(gisbase, 'lib')
    if path:
        path = dir + os.pathsep + path
    else:
        path = dir
    os.environ['LD_LIBRARY_PATH'] = path

    # languageQUIET
    os.environ['LANG'] = 'en_US'
    os.environ['LOCALE'] = 'C'

    import grass.script.setup as gsetup
    import grass.script as gscript

    gsetup.init(gisbase, gisdb, location, mapset)
    return gscript