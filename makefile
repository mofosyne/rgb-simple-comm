APPROACHES = approaches/transition-clocked approaches/dark-clocked

all:
	$(foreach d,$(APPROACHES),$(MAKE) -C $(d);)

clean:
	$(foreach d,$(APPROACHES),$(MAKE) -C $(d) clean;)

test:
	$(foreach d,$(APPROACHES),$(MAKE) -C $(d) test;)
