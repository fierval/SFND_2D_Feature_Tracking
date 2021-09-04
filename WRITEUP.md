# Camera Based 2D Feature Tracking

Project writeup

## General Refactoring

I have refactored the main keypoint matching function into `run_data_collection` to be able to easily assemble the necessary results

## Data Buffer Optimization

I would normally use `boost` for this with the data structure already implemented, however, since there is not that much code to write here and it's used only once, and the grader probably doesn't have `boost` installed, I just used the `std::vector` with `std::rotate` to implement the cyclic buffer.
