#ifndef LASSIE_WIN_FILE_ASSOCIATION_HPP
#define LASSIE_WIN_FILE_ASSOCIATION_HPP

namespace dissco {
namespace win {

// Registers .dissco -> this LASSIE.exe under HKCU\Software\Classes so that
// double-clicking a .dissco file in Explorer launches LASSIE.
//
// Per-user (no admin), idempotent (skips work if HKCU already points at this
// exe), and a no-op outside of Windows builds with LASSIE_CLAIM_DISSCO set
// at compile time. The compile-time gate matches the macOS / Linux behavior:
// debug / worktree builds never write the registry, so multiple in-flight
// LASSIE checkouts won't fight over the .dissco extension.
void registerDisscoAssociation();

} // namespace win
} // namespace dissco

#endif // LASSIE_WIN_FILE_ASSOCIATION_HPP
