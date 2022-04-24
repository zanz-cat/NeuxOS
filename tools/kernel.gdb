source tools/.gdbinit

define dent_list
    set $dent = $arg0
    set $node = (&$dent->subdirs)->next
    while $node != &$dent->subdirs
        set $d = container_of($node, struct dentry, child)
        print $d
        print *$d
        set $node = $node->next
    end
end

define heap_list
    set $block = $arg0->next
    while $block != heap
        print *$block
        set $block = $block->next
    end
end