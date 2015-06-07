#
# By default V8 creates thin archives which simply reference their object files, rather than
# normal archive include their object files.
# 
# This script rebuilds the archives as normal archives.
#
# See http://stackoverflow.com/questions/25554621/turn-thin-archive-into-normal-one
#
for lib in `find . -name '*.a'`;
    do arm-linux-androideabi-ar -t $lib | xargs arm-linux-androideabi-ar rvs $lib.new && mv -v $lib.new $lib;
done
