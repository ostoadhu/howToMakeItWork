import pyatspi
import sys

def get_tree_data(obj, indent=0):
    """Recursively collects role and name data from the tree."""
    tree_lines = []
    try:
        role = obj.getRoleName()
        name = obj.getName()
        
        # Format the current node
        line = "  " * indent + f"[{role}] {name}"
        tree_lines.append(line)

        # Iterate through children
        for i in range(obj.getChildCount()):
            child = obj.getChildAtIndex(i)
            tree_lines.extend(get_tree_data(child, indent + 1))
    except Exception:
        # Some nodes might be transient (disappear during crawl)
        pass
    return tree_lines

def main():
    # 1. Connect to the desktop registry
    reg = pyatspi.Registry
    desktop = reg.getDesktop(0)

    # 2. Find the Chromium/Chrome application
    target_app = None
    for app in desktop:
        if "chrom" in app.get_name().lower():
            target_app = app
            break

    if not target_app:
        print("Error: Chromium not found. Make sure it's open with --force-renderer-accessibility")
        return

    # 3. Find the 'document web' node specifically
    # Webpages are usually inside a 'frame', then a 'document web'
    def find_document(node):
        if node.getRoleName() == "document web":
            return node
        for i in range(node.getChildCount()):
            res = find_document(node.getChildAtIndex(i))
            if res: return res
        return None

    print(f"Searching for webpage content in {target_app.get_name()}...")
    doc_node = find_document(target_app)

    if doc_node:
        print("Document found! Collecting tree...")
        full_tree = get_tree_data(doc_node)
        
        with open("webpage_tree.txt", "w", encoding="utf-8") as f:
            f.write("\n".join(full_tree))
        print(f"Success! Saved {len(full_tree)} nodes to webpage_tree.txt")
    else:
        print("Could not find a 'document web' node. Is the tab loaded?")

if __name__ == "__main__":
    main()
