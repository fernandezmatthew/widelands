push_textdomain("world")

dirname = path.dirname(__file__)

wl.Descriptions():new_immovable_type{
   name = "snowman",
   descname = _("Snowman"),
   size = "none",
   programs = {},
   animation_directory = dirname,
   animations = {
      idle = {
         hotspot = { 9, 24 },
      },
   }
}

pop_textdomain()
