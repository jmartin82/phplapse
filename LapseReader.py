import urwid
import random
import struct
import getopt
import sys
import os

class LapseReader:

    def __init__(self, filename, path):
        self.idxFile = open(filename, mode='rb')
        self._loadHeader()
        self.headerSize = 96
        self.currentStep = 0

    def getStepData(self,step):
        self.currentStep = step

        offset = self.headerSize + step * 64;
        self.idxFile.seek(offset,0)

        num = struct.unpack('<L', self.idxFile.read(4))[0]
        ltime = struct.unpack('<L', self.idxFile.read(4))[0]
        ttime = struct.unpack('<L', self.idxFile.read(4))[0]
        mem = struct.unpack('<L', self.idxFile.read(4))[0]
        mem_peak = struct.unpack('<L', self.idxFile.read(4))[0]
        fname = struct.unpack('20s', self.idxFile.read(20))[0].rstrip('\x00')
        cname = struct.unpack('20s', self.idxFile.read(20))[0].rstrip('\x00')
        context_index = struct.unpack('<L', self.idxFile.read(4))[0]


        self.datFile.seek(context_index,0)

        filename = self.datFile.readline().rstrip('\n')

        N=11
        context =[]
        context.extend(self.datFile.readline() for i in xrange(N))


        return {"num" : num,
                "ltime" : ltime,
                "ttime" : ttime,
                "mem" : mem, 
                "mem_peak" : mem_peak,
                "fname" : fname,
                "cname" : cname,
                "filename" : filename,
                "context" : context}

    def rewind(self):
        return self.getStepData(0)


    def goNext(self):
        if (self.currentStep<self.steps):
            self.currentStep+=1
        return self.getStepData(self.currentStep)

    def goBack(self):
        if (self.currentStep>0):
            self.currentStep-=1
        return self.getStepData(self.currentStep)

    def _loadHeader(self):
        print "Reading header"
        self.version = struct.unpack('<H', self.idxFile.read(2))[0]
        self.idxFileName = struct.unpack('70s', self.idxFile.read(70))[0].rstrip('\x00')
        self.datFile = open(path+"/"+self.idxFileName, mode='rb')
        self.date = struct.unpack('20s', self.idxFile.read(20))[0].rstrip('\x00')
        self.steps = struct.unpack('<L', self.idxFile.read(4))[0]

    def __str__(self):
        return "Version " + str(self.version) + "\nDate " + self.date +  "\nSteps " + str(self.steps) + "\n"
  
######################################################################      

def main (lapseReader):

    palette = [
        ('body','white', '', 'standout'),
        ('context','dark green', '', 'standout'),
        ('context run','dark cyan', '', 'standout'),
        ('focus','dark red', '', 'standout'),
        ('head','light red', 'black'),
        ('footer','light red', 'black'),
        ('pg normal',    'white',      'black', 'standout'),
        ('pg complete',  'white',      'light cyan'),
        ]


    def keystroke (input):
        if input in ('q', 'Q'):
            raise urwid.ExitMainLoop()

        if input in ('r', 'R'):
            step = lapseReader.rewind()
            writeStep(step)

        if input is 'left':
            step = lapseReader.goBack()
            writeStep(step)

        if input is 'right':
            step = lapseReader.goNext()
            writeStep(step)

    def writeStep(step):
        steps.set_text("Step: "+str(step['num'])+"/"+str(lapseReader.steps))
        contextCode.set_text(step['context'])
        filename.set_text('File: ' + step['filename'])
        time.set_text('Time: '+str(step['ltime']) +'us')
        totalTime.set_text('Total Time: ' +str(step['ttime']/1000) +'ms')
        mem.set_text('Mem: '+str(step['mem'])+'Kb')
        memPeak.set_text('Mem Peak: '+str(step['mem_peak'])+'Kb')
        cname.set_text('Class: '+step['cname'])
        fname.set_text('Function: '+step['fname'])
        prg.set_completion(step['num']-1)


    header = urwid.AttrMap(urwid.Text('PHPLapse Viewer 1.0'), 'head')

    contextCode = urwid.Text('\n\n\n\n\n\n\n\n\n\n')
    center = urwid.AttrMap(contextCode,'context')
    center = urwid.LineBox(center)


    filename = urwid.Text('File: ')

    steps = urwid.Text('Steps: ')
    cname = urwid.Text('Class: ')
    fname = urwid.Text('Function: ')
    oo = urwid.Columns([steps,cname,fname])

    time = urwid.Text('Time: 0ms')
    dtime = urwid.AttrMap(time,'context run')

    totalTime = urwid.Text('Total Time: 0ms')
    dtotalTime = urwid.AttrMap(totalTime,'context run')

    mem = urwid.Text('Mem: 0Kb')
    dmem = urwid.AttrMap(mem,'context run')

    memPeak = urwid.Text('Mem Peak: 0Kb')
    dmemPeak = urwid.AttrMap(memPeak,'context run')


    contextRun = urwid.AttrWrap(urwid.Columns([dtime,dtotalTime,dmem,dmemPeak],4), 'bright')



    prg =  urwid.ProgressBar('pg normal','pg complete',current=0, done=lapseReader.steps)



    l = [ urwid.Divider("="), contextRun , urwid.Divider("="), oo  , urwid.Divider("="), filename, urwid.Divider("="), center , urwid.Divider("="), urwid.Divider(),prg  ]
    w = urwid.ListBox(urwid.SimpleListWalker(l))

    # Frame
    w = urwid.AttrWrap(w, 'body')


    footer = urwid.Columns([urwid.Text('[Q]: Quit'),urwid.Text('[R]: Rewind'),urwid.Text('[<]: Back'),urwid.Text('[>]: Next')],4)
    footer = urwid.AttrMap(footer, 'footer')


    view = urwid.Frame(body=w, header=header, footer=footer)
    loop = urwid.MainLoop(view, palette, unhandled_input=keystroke)

    #load first step
    writeStep(lapseReader.getStepData(0))

    loop.run()

def usage():
    print "LapseReader -i index_file.idx [-p path_of_data_file]\n"


if __name__ == '__main__':
    try:                                
        opts, args = getopt.getopt(sys.argv[1:], "hi:p", ["help", "input=", "path="])
    except getopt.GetoptError:
        usage()
        sys.exit(2)

    path = False
    index_file = False
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            usage()                  
            sys.exit()              
        elif opt in ("-i", "--input"):
            index_file = arg
        elif opt in ("-p", "--path"):
            path = arg
    
    if (index_file != False and path == False) :
        path = os.path.dirname(index_file)

    if (path==False or index_file==False):
        usage() 
        sys.exit()

    lapseReader = LapseReader(index_file,path)
    print str(lapseReader)
    main(lapseReader)