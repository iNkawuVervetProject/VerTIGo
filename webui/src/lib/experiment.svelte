<script lang="ts">
 import { type Experiment } from '$lib/types';

 export let experiment: Experiment | undefined = undefined;
</script>

<p> Title: {experiment?.key} </p>
