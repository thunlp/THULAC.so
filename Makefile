dst_dir=.
src_dir=src
cxx=g++ -O3 -I $(src_dir)
cxxd=g++ -g -I $(src_dir)

# all: dst $(dst_dir)/thulac $(dst_dir)/libthulac.so
all: dst $(dst_dir)/libthulac.so

dst:
	@if [ ! -d $(dst_dir) ]; then mkdir -p $(dst_dir); fi

$(dst_dir)/thulac: $(src_dir)/thulac.cc $(src_dir)/*.h
	$(cxx) $(src_dir)/thulac.cc -o $(dst_dir)/thulac

$(dst_dir)/libthulac.so: $(src_dir)/thulac_lib.cc $(src_dir)/test.cc $(src_dir)/*.h
	$(cxx) $(src_dir)/thulac_lib.cc -shared -fPIC -Wl,-soname=libthulac.so.1 -o $(dst_dir)/libthulac.so

clean:
	# rm $(dst_dir)/thulac
	rm $(dst_dir)/libthulac.so

pack:
	tar -czvf thulac_linux_c++_v1.tar.gz models src Makefile README.md
