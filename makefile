APPROACHES = approaches/transition-clocked approaches/dark-clocked approaches/human-readable

all:
	$(foreach d,$(APPROACHES),$(MAKE) -C $(d);)

clean:
	$(foreach d,$(APPROACHES),$(MAKE) -C $(d) clean;)

test:
	$(foreach d,$(APPROACHES),$(MAKE) -C $(d) test;)
