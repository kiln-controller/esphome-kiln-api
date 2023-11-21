import Schedules from './routes/Schedules.svelte'
import Kiln from './routes/Kiln.svelte'
import NotFound from './routes/NotFound.svelte'

// Export the route definition object
export default {
    '/': Schedules,
    '/kiln': Kiln,
    // Catch-all, must be last
    '*': NotFound,
}