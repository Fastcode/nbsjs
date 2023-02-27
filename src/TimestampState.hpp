#ifndef NBS_TIMESTAMPSTATE_HPP
#define NBS_TIMESTAMPSTATE_HPP


namespace nbs {
    struct TimestampState {
        int jumps;
        std::vector<IndexItemFile>::iterator position;
        std::vector<IndexItemFile>::iterator begin;
        std::vector<IndexItemFile>::iterator end;
    };
}  // namespace nbs

#endif
