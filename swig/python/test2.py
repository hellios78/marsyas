import marsyas 

mng = marsyas.MarSystemManager() 

# print mng.registeredPrototypes()


pnet = mng.create("Series", "pnet")
src = mng.create("SineSource", "src")
# src = mng.create("SoundFileSource", "src");
gain = mng.create("Gain", "gain") 
#dest = mng.create("SoundFileSink", "dest") 
dest = mng.create("AudioSink", "dest")     

pnet.addMarSystem(src)
pnet.addMarSystem(gain) 
pnet.addMarSystem(dest)


# pnet.updControl("SoundFileSource/src/mrs_string/filename", "/Users/gtzan/data/sound/music_speech/music/gravity.au")
pnet.updControl("SineSource/src/mrs_real/frequency", marsyas.MarControlPtr.from_real(999))
print pnet.getControl("SineSource/src/mrs_real/frequency").to_real()

# Audio has lots of clicks and seems to crash - not sure why 
pnet.updControl("AudioSink/dest/mrs_bool/initAudio", marsyas.MarControlPtr.from_bool(True))

# pnet.updControl("SoundFileSink/dest/mrs_string/filename", "foo.wav")
# print pnet.getControl("SoundFileSink/dest/mrs_string/filename")

pnet.linkControl("mrs_real/frequency", "SineSource/src/mrs_real/frequency");


for j in range (1,12): 
    pnet.updControl("mrs_real/frequency", marsyas.MarControlPtr.from_real(j * 220.0))
#    pnet.updControl("SineSource/src/mrs_real/frequency", j * 220.0)

    for i in range (1,10):	
    	    	pnet.tick()



