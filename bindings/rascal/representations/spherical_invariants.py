import json

from .base import CalculatorFactory, cutoff_function_dict_switch
from ..neighbourlist import AtomsList
import numpy as np
from copy import deepcopy
from ..utils import BaseIO


class SphericalInvariants(BaseIO):

    """
    Computes a SphericalInvariants representation, i.e. the SOAP power spectrum.

    Hyperparameters
    ----------
    interaction_cutoff : float
        Maximum pairwise distance for atoms to be considered in
        expansion

    cutoff_smooth_width : float
        The distance over which the the interaction is smoothed to zero

    max_radial : int
        Number of radial basis functions

    max_angular : int
        Highest angular momentum number (l) in the expansion

    gaussian_sigma_type : str
        How the Gaussian atom sigmas (smearing widths) are allowed to
        vary -- fixed ('Constant'), by species ('PerSpecies'), or by
        distance from the central atom ('Radial').

    gaussian_sigma_constant : float
        Specifies the atomic Gaussian widths, in the case where they're
        fixed.

    soap_type : string
        Specifies the type of representation to be computed
        (power spectrum etc.).

    inversion_symmetry : Boolean
        Specifies whether inversion invariance should be enforced.
        (Only relevant for BiSpectrum.)

    normalize : boolean
        Whether to normalize so that the kernel between identical environments
        is 1.  Default and highly recommended: True.

    expansion_by_species_method : string
        Specifies the how the species key of the invariant are set-up.
        Possible values: 'environment wise', 'user defined', 'structure wise'.
        The descriptor is computed for each atomic enviroment and it is indexed
        using tuples of atomic species that are present within the environment.
        This index is by definition sparse since a species tuple will be non
        zero only if the atomic species are present inside the environment.
        'environment wise' means that each environmental representation
        will only contain the minimal set of species tuples needed by each
        atomic environment.
        'structure wise' means that within a structure the species tuples
        will be the same for each environment coefficients.
        'user defined' uses global_species to set-up the species tuples.

        These different settings correspond to different trade-off between
        the memory efficiency of the invariants and the computational
        efficiency of the kernel computation.
        When computing a kernel using 'environment wise' setting does not allow
        for efficent matrix matrix multiplications which is ensured when
        'user defined' is used. 'structure wise' is a balance between the
        memory footprint and the use of matrix matrix products.

        Note that the sparsity of the gradient coefficients and their use to
        build kernels does not allow for clear efficiency gains so their
        sparsity is kept irrespective of expansion_by_species_method.

    global_species : list
        list of species to use to set-up the species key of the invariant. It
        should contain all the species present in the structure for which
        invariants will be computed

    Methods
    -------
    transform(frames)
        Compute the representation for a list of ase.Atoms object.


    .. [soap] Bartók, Kondor, and Csányi, "On representing chemical
        environments", Phys. Rev. B. 87(18), p. 184115
        http://link.aps.org/doi/10.1103/PhysRevB.87.184115

    """

    def __init__(self, interaction_cutoff, cutoff_smooth_width,
                 max_radial, max_angular, gaussian_sigma_type,
                 gaussian_sigma_constant=0.,
                 cutoff_function_type="ShiftedCosine",
                 soap_type="PowerSpectrum", inversion_symmetry=True,
                 radial_basis="GTO", normalize=True,
                 optimization_args={},
                 expansion_by_species_method="environment wise",
                 global_species=None,
                 cutoff_function_parameters=dict(),
                 compute_gradients=False):
        """Construct a SphericalExpansion representation

        Required arguments are all the hyperparameters named in the
        class documentation
        """
        self.name = 'sphericalinvariants'
        self.hypers = dict()
        if global_species is None:
            global_species = []
        elif not isinstance(global_species, list):
            global_species = list(global_species)

        self.update_hyperparameters(
            max_radial=max_radial, max_angular=max_angular,
            soap_type=soap_type, normalize=normalize,
            inversion_symmetry=inversion_symmetry,
            expansion_by_species_method=expansion_by_species_method,
            global_species=global_species, compute_gradients=compute_gradients)
        self.cutoff_function_parameters = deepcopy(cutoff_function_parameters)
        cutoff_function_parameters.update(
            interaction_cutoff=interaction_cutoff,
            cutoff_smooth_width=cutoff_smooth_width
        )
        cutoff_function = cutoff_function_dict_switch(
            cutoff_function_type, **cutoff_function_parameters)

        gaussian_density = dict(
            type=gaussian_sigma_type,
            gaussian_sigma=dict(
                value=gaussian_sigma_constant,
                unit='AA'
            ),
        )
        self.optimization_args = deepcopy(optimization_args)
        if 'type' in optimization_args:
            if optimization_args['type'] == 'Spline':
                if 'accuracy' in optimization_args:
                    accuracy = optimization_args['accuracy']
                else:
                    accuracy = 1e-8
                if 'range' in optimization_args:
                    spline_range = optimization_args['range']
                else:
                    # TODO(felix) remove this when there is a check for the
                    # distance for the usage of the interpolator in the
                    # RadialContribution
                    print("Warning: default parameter for spline range is used.")
                    spline_range = (0, interaction_cutoff)
                optimization_args = {
                    'type': 'Spline', 'accuracy': accuracy, 'range': {
                        'begin': spline_range[0], 'end': spline_range[1]}}
            elif optimization_args['type'] == 'None':
                optimization_args = dict({'type': 'None'})
            else:
                print('Optimization type is not known. Switching to no'
                      ' optimization.')
                optimization_args = dict({'type': 'None'})
        else:
            optimization_args = dict({'type': 'None'})

        radial_contribution = dict(
            type=radial_basis,
            optimization=optimization_args
        )

        self.update_hyperparameters(cutoff_function=cutoff_function,
                                    gaussian_density=gaussian_density,
                                    radial_contribution=radial_contribution)

        if soap_type == "RadialSpectrum":
            self.update_hyperparameters(max_angular=0)

        self.nl_options = [
            dict(name='centers', args=[]),
            dict(name='neighbourlist', args=dict(cutoff=interaction_cutoff)),
            dict(name="centercontribution", args=dict()),
            dict(name='strict', args=dict(cutoff=interaction_cutoff))
        ]

        self.rep_options = dict(name=self.name, args=[self.hypers])

        self._representation = CalculatorFactory(self.rep_options)

    def update_hyperparameters(self, **hypers):
        """Store the given dict of hyperparameters

        Also updates the internal json-like representation

        """
        allowed_keys = {'interaction_cutoff', 'cutoff_smooth_width',
                        'max_radial', 'max_angular', 'gaussian_sigma_type',
                        'gaussian_sigma_constant', 'soap_type',
                        'inversion_symmetry', 'cutoff_function', 'normalize',
                        'gaussian_density', 'radial_contribution',
                        'cutoff_function_parameters', 'compute_gradients', 'expansion_by_species_method', 'global_species'}
        hypers_clean = {key: hypers[key] for key in hypers
                        if key in allowed_keys}
        self.hypers.update(hypers_clean)
        return

    def transform(self, frames):
        """Compute the representation.

        Parameters
        ----------
        frames : list(ase.Atoms)
            List of atomic structures.

        Returns
        -------
            Object containing the representation

        """
        if not isinstance(frames, AtomsList):
            frames = AtomsList(frames, self.nl_options)

        self._representation.compute(frames.managers)

        return frames

    def get_num_coefficients(self, n_species=1):
        """Return the number of coefficients in the representation

        (this is the descriptor size per atomic centre)

        """
        if self.hypers['soap_type'] == 'RadialSpectrum':
            return (n_species * self.hypers['max_radial'])
        if self.hypers['soap_type'] == 'PowerSpectrum':
            return (int((n_species *
                         (n_species +
                          1)) /
                        2) *
                    self.hypers['max_radial']**2 *
                    (self.hypers['max_angular'] +
                     1))
        if self.hypers['soap_type'] == 'BiSpectrum':
            if not self.hypers['inversion_symmetry']:
                return (n_species**3
                        * self.hypers['max_radial']**3
                        * int(1 + 2 * self.hypers['max_angular']
                              + 3 * self.hypers['max_angular']**2 / 2
                              + self.hypers['max_angular']**3 / 2))
            else:
                return (n_species**3
                        * self.hypers['max_radial']**3
                        * int(np.floor(((self.hypers['max_angular'] + 1)**2 + 1)
                                       * (2 * (self.hypers['max_angular'] + 1) + 3) / 8.0)))
        else:
            raise ValueError('Only soap_type = RadialSpectrum || '
                             'PowerSpectrum || BiSpectrum '
                             'implemented for now')

    def get_keys(self, species):
        """
        return the proper list of keys used to build the representation
        """
        keys = []
        if self.hypers['soap_type'] == 'RadialSpectrum':
            for sp in species:
                keys.append([sp])
        elif self.hypers['soap_type'] == 'PowerSpectrum':
            for sp1 in species:
                for sp2 in species:
                    if sp1 > sp2:
                        continue
                    keys.append([sp1, sp2])
        elif self.hypers['soap_type'] == 'BiSpectrum':
            for sp1 in species:
                for sp2 in species:
                    if sp1 > sp2:
                        continue
                    for sp3 in species:
                        if sp2 > sp3:
                            continue
                        keys.append([sp1, sp2, sp3])
        else:
            raise ValueError('Only soap_type = RadialSpectrum || '
                             'PowerSpectrum || BiSpectrum '
                             'implemented for now')
        return keys

    def get_init_params(self):
        gaussian_density = self.hypers['gaussian_density']
        cutoff_function = self.hypers['cutoff_function']
        radial_contribution = self.hypers['radial_contribution']

        init_params = dict(
            interaction_cutoff=cutoff_function['cutoff']['value'],
            cutoff_smooth_width=cutoff_function['smooth_width']['value'],
            max_radial=self.hypers['max_radial'], max_angular=self.hypers['max_angular'],
            soap_type=self.hypers['soap_type'],
            inversion_symmetry=self.hypers['inversion_symmetry'],
            normalize=self.hypers['normalize'],
            expansion_by_species_method=self.hypers['expansion_by_species_method'],
            global_species=self.hypers['global_species'],
            compute_gradients=self.hypers['compute_gradients'],
            gaussian_sigma_type=gaussian_density['type'],
            gaussian_sigma_constant=gaussian_density['gaussian_sigma']['value'],
            cutoff_function_type=cutoff_function['type'],
            radial_basis=radial_contribution['type'],
            optimization_args=self.optimization_args,
            cutoff_function_parameters=self.cutoff_function_parameters,
        )
        return init_params

    def _set_data(self, data):
        pass

    def _get_data(self):
        return dict()
