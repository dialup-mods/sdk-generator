#pragma once

struct DeleteRange {
    size_t start;
    size_t end;
};

std::vector<DeleteRange> toDelete;

// Mark this class's byte range for deletion
//        toDelete.push_back({
//            schema->sourceStartOffset,
//            schema->sourceEndOffset
//        });
        // ... emit merged version
    }
}

// Delete in reverse order (so offsets stay valid)
std::sort(toDelete.begin(), toDelete.end(),
    [](auto& a, auto& b) { return a.start > b.start; });

std::string schemaContents = readFile("Schema.h");
for (auto& range : toDelete) {
    schemaContents.erase(range.start, range.end - range.start);
}

writeFile("FinalSchema.h", schemaContents);